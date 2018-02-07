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
getEntities.cpp
postEntities.cpp
entryPointsTreat.cpp
getEntity.cpp
putEntity.cpp
patchEntity.cpp
getEntityAttribute.cpp
putEntityAttribute.cpp
getEntityAttributeValue.cpp
putEntityAttributeValue.cpp
postEntity.cpp
deleteEntity.cpp
getEntityType.cpp
getEntityAllTypes.cpp
badVerbGetPutOnly.cpp
getAllSubscriptions.cpp
getSubscription.cpp
postSubscriptions.cpp
deleteSubscription.cpp
patchSubscription.cpp
postBatchQuery.cpp
badVerbGetDeletePatchOnly.cpp
postBatchUpdate.cpp
logLevelTreat.cpp
badVerbAllNotDelete.cpp
semStateTreat.cpp
getMetrics.cpp
deleteMetrics.cpp
getRegistration.cpp
getRegistrations.cpp
optionsGetOnly.cpp
optionsGetPostOnly.cpp
postRegistration.cpp
optionsGetDeleteOnly
optionsAllNotDelete
optionsGetPutOnly
optionsGetPutDeleteOnly
optionsGetDeletePatchOnly
optionsPostOnly
)

SET (HEADERS
getEntities.h
postEntities.h
entryPointsTreat.h
getEntity.h
putEntity.h
patchEntity.h
getEntityAttribute.h
putEntityAttribute.h
getEntityAttributeValue.h
putEntityAttributeValue.h
postEntity.h
deleteEntity.h
getEntityType.h
getEntityAllTypes.h
badVerbGetPutOnly.h
getAllSubscriptions.h
getSubscription.h
postSubscriptions.h
deleteSubscription.h
patchSubscription.h
postBatchQuery.h
badVerbGetDeletePatchOnly.h
postBatchUpdate.h
logLevelTreat.h
badVerbAllNotDelete.h
semStateTreat.h
getMetrics.h
deleteMetrics.h
optionsGetOnly.h
optionsGetPostOnly.h
getRegistration.h
getRegistrations.h
postRegistration.h
)



# Include directories
# -----------------------------------------------------------------
include_directories("${PROJECT_SOURCE_DIR}/src/lib")


# Library declaration
# -----------------------------------------------------------------
ADD_LIBRARY(serviceRoutinesV2 STATIC ${SOURCES} ${HEADERS})
