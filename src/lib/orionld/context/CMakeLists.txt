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
    orionldCoreContext.cpp
    orionldContextCreate.cpp
    orionldContextInit.cpp
    orionldContextCache.cpp
    orionldContextPresent.cpp
    orionldContextTreePresent.cpp
    orionldContextHashTablesFill.cpp
    orionldContextDownload.cpp
    orionldContextItemAliasLookup.cpp
    orionldContextItemValueLookup.cpp
    orionldContextPrefixExpand.cpp
    orionldContextItemLookup.cpp
    orionldContextValueExpand.cpp
    orionldContextItemExpand.cpp
    orionldContextFromBuffer.cpp
    orionldContextFromUrl.cpp
    orionldContextSimplify.cpp
    orionldContextFromTree.cpp
    orionldContextFromObject.cpp
    orionldContextCacheInsert.cpp
    orionldContextUrlGenerate.cpp
    orionldContextFromArray.cpp
    orionldContextCacheInit.cpp
    orionldContextCacheLookup.cpp
    orionldContextCachePresent.cpp
    orionldContextCacheGet.cpp
)

# Include directories
# -----------------------------------------------------------------
include_directories("${PROJECT_SOURCE_DIR}/src/lib")


# Library declaration
# -----------------------------------------------------------------
ADD_LIBRARY(orionld_context STATIC ${SOURCES})
