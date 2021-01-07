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

CMAKE_MINIMUM_REQUIRED(VERSION 2.6)

SET (SOURCES
    troe.cpp
    troeInit.cpp
    troeDeleteAttribute.cpp
    troeDeleteEntity.cpp
    troePatchAttribute.cpp
    troePatchEntity.cpp
    troePostBatchCreate.cpp
    troePostBatchDelete.cpp
    troePostBatchUpdate.cpp
    troePostBatchUpsert.cpp
    troePostEntities.cpp
    troePostEntity.cpp
    troePostEntityNoOverwrite.cpp
    troePatchAttribute.cpp
    pgInit.cpp
    pgConnectionGet.cpp
    pgConnectionRelease.cpp
    pgDatabaseCreate.cpp
    pgDatabasePrepare.cpp
    pgDatabaseTableExists.cpp
    pgDatabaseTableCreateAll.cpp
    pgTransactionBegin.cpp
    pgTransactionCommit.cpp
    pgTransactionRollback.cpp
    pgEntityTreat.cpp
    pgEntityPush.cpp
    pgAttributeTreat.cpp
    pgAttributePush.cpp
    pgSubAttributePush.cpp
    pgRelationshipPush.cpp
    pgStringPropertyPush.cpp
    pgNumberPropertyPush.cpp
    pgCompoundPropertyPush.cpp
    pgBoolPropertyPush.cpp
    pgSubAttributeTreat.cpp
    pgSubRelationshipPush.cpp
    pgStringSubPropertyPush.cpp
    pgBoolSubPropertyPush.cpp
    pgCompoundSubPropertyPush.cpp
    pgNumberSubPropertyPush.cpp
    pgEntityDelete.cpp
    pgAttributeDelete.cpp
    pgGeoPropertyPush.cpp
    pgGeoPointPush.cpp
    pgGeoPolygonPush.cpp
    pgGeoMultiPolygonPush.cpp
    pgGeoLineStringPush.cpp
)

SET (HEADERS
    troe.h
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
    troePostEntityNoOverwrite.h
    troePatchAttribute.h
    pgInit.h
    pgConnectionGet.h
    pgConnectionRelease.h
    pgDatabaseCreate.h
    pgDatabasePrepare.h
    pgDatabaseTableExists.h
    pgDatabaseTableCreateAll.h
    pgTransactionBegin.h
    pgTransactionCommit.h
    pgTransactionRollback.h
    pgEntityTreat.h
    pgEntityPush.h
    pgAttributeTreat.h
    pgAttributePush.h
    pgSubAttributePush.h
    pgRelationshipPush.h
    pgNumberPropertyPush.h
    pgCompoundPropertyPush.h
    pgBoolPropertyPush.h
    pgStringSubPropertyPush.h
    pgBoolSubPropertyPush.h
    pgCompoundSubPropertyPush.h
    pgNumberSubPropertyPush.h
    pgEntityDelete.h
    pgAttributeDelete.h
    pgGeoPropertyPush.h
    pgGeoPointPush.h
    pgGeoPolygonPush.h
    pgGeoMultiPolygonPush.h
    pgGeoLineStringPush.h
)


# Include directories
# -----------------------------------------------------------------
include_directories("${PROJECT_SOURCE_DIR}/src/lib")


# Library declaration
# -----------------------------------------------------------------
ADD_LIBRARY(orionld_troe STATIC ${SOURCES} ${HEADERS})
