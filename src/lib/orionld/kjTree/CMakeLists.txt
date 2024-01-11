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
    kjTreeToTimeInterval.cpp
    kjTreeToGeoLocation.cpp
    kjStringValueLookupInArray.cpp
    kjEntityIdLookupInEntityArray.cpp
    kjTreeRegistrationInfoExtract.cpp
    kjEntityIdArrayExtract.cpp
    kjChildAddOrReplace.cpp
    kjGeojsonEntityTransform.cpp
    kjGeojsonEntitiesTransform.cpp
    kjEntityArrayErrorPurge.cpp
    kjStringArraySortedInsert.cpp
    kjSort.cpp
    kjChildCount.cpp
    kjEntityNormalizedToConcise.cpp
    kjEntityNormalizedToSimplified.cpp
    kjTreeLog.cpp
    kjTimestampAdd.cpp
    kjArrayAdd.cpp
    kjChildPrepend.cpp
    kjNodeDecouple.cpp
    kjNavigate.cpp
    kjTreeFromCachedSubscription.cpp
    kjTreeFromPernotSubscription.cpp
    kjAttributeNormalizedToSimplified.cpp
    kjAttributeNormalizedToConcise.cpp
    kjSysAttrsRemove.cpp
    kjLookupInKvList.cpp
)

# Include directories
# -----------------------------------------------------------------
include_directories("${PROJECT_SOURCE_DIR}/src/lib")


# Library declaration
# -----------------------------------------------------------------
ADD_LIBRARY(orionld_kjTree STATIC ${SOURCES})
