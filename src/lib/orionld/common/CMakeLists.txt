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

CMAKE_MINIMUM_REQUIRED(VERSION 3.5)

SET (SOURCES
    orionldRequestSend.cpp
    urlParse.cpp
    linkCheck.cpp
    httpStatusCodeToOrionldErrorType.cpp
    numberToDate.cpp
    orionldState.cpp
    uuidGenerate.cpp
    orionldServerConnect.cpp
    dotForEq.cpp
    eqForDot.cpp
    entitySuccessPush.cpp
    entityErrorPush.cpp
    entityIdCheck.cpp
    entityTypeCheck.cpp
    entityLookupById.cpp
    removeArrayEntityLookup.cpp
    typeCheckForNonExistingEntities.cpp
    entityIdAndTypeGet.cpp
    orionldTenantInit.cpp
    orionldTenantLookup.cpp
    orionldTenantCreate.cpp
    orionldTenantGet.cpp
    attributeUpdated.cpp
    attributeNotUpdated.cpp
    isSpecialSubAttribute.cpp
    duplicatedInstances.cpp
    troeIgnored.cpp
    tenantList.cpp
    attributeTypeFromUriParam.cpp
    orionldError.cpp
    orionldPatchApply.cpp
    langStringExtract.cpp
    pathComponentsSplit.cpp
    urlDecode.cpp
    subCacheApiSubscriptionInsert.cpp
    batchEntityCountAndFirstCheck.cpp
    batchEntityStringArrayPopulate.cpp
    batchEntitiesFinalCheck.cpp
    batchMultipleInstances.cpp
    batchCreateEntity.cpp
    batchUpdateEntity.cpp
    batchReplaceEntity.cpp
    curlToBrokerStrerror.cpp
    fileName.cpp
    responseFix.cpp
    lowercase.cpp
    stringStrip.cpp
    dateTime.cpp
    forbidden.cpp
)

# Include directories
# -----------------------------------------------------------------
include_directories("${PROJECT_SOURCE_DIR}/src/lib")


# Library declaration
# -----------------------------------------------------------------
ADD_LIBRARY(orionld_common STATIC ${SOURCES})
