# Copyright 2022 FIWARE Foundation e.V.
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
    distOpInit.cpp
    distOpSend.cpp
    distOpsSend.cpp
    distOpListsMerge.cpp
    distOpListRelease.cpp
    distOpListDebug.cpp
    distOpSuccess.cpp
    distOpFailure.cpp
    distOpLookupByCurlHandle.cpp
    distOpLookupByRegId.cpp
    distOpEntityMerge.cpp
    distOpRequests.cpp
    distOpResponses.cpp
    distOpCreate.cpp
    distOpAttrs.cpp
    distOpItemListDebug.cpp
    distOpListItemCreate.cpp
    distOpListItemAdd.cpp
    distOpResponseMergeIntoEntityArray.cpp
    distOpsReceive2.cpp
    distOpsSendAndReceive.cpp
    xForwardedForCompose.cpp
    xForwardedForMatch.cpp
    viaCompose.cpp
    viaMatch.cpp
)

# Include directories
# -----------------------------------------------------------------
include_directories("${PROJECT_SOURCE_DIR}/src/lib")


# Library declaration
# -----------------------------------------------------------------
ADD_LIBRARY(orionld_distOp STATIC ${SOURCES})
