# Copyright 2018 FIWARE Foundation e.V.
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
    orionldRequestSend.cpp
    orionldErrorResponse.cpp
    urlParse.cpp
    urlCheck.cpp
    urnCheck.cpp
    linkCheck.cpp
    httpStatusCodeToOrionldErrorType.cpp
    geoJsonCreate.cpp
    numberToDate.cpp
    orionldState.cpp
    QNode.cpp
    qLex.cpp
    qLexCheck.cpp
    qLexRender.cpp
    qParse.cpp
    qTreePresent.cpp
    qTreeToBsonObj.cpp
    uuidGenerate.cpp
    orionldServerConnect.cpp
    dotForEq.cpp
    eqForDot.cpp
    entityErrorPush.cpp
    entityIdCheck.cpp
    entityTypeCheck.cpp
    entityLookupById.cpp
    removeArrayEntityLookup.cpp
    typeCheckForNonExistingEntities.cpp
    entityIdAndTypeGet.cpp
    qAliasCompact.cpp
    orionldTenantLookup.cpp
    orionldTenantCreate.cpp
    # qTreeToBson.cpp
)

# Include directories
# -----------------------------------------------------------------
include_directories("${PROJECT_SOURCE_DIR}/src/lib")


# Library declaration
# -----------------------------------------------------------------
ADD_LIBRARY(orionld_common STATIC ${SOURCES})
