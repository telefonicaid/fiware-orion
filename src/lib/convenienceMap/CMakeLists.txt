# Copyright 2013 Telefonica Investigacion y Desarrollo, S.A.U
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
# fermin at tid dot es

CMAKE_MINIMUM_REQUIRED(VERSION 2.6)

SET (SOURCES
    mapGetContextEntitiesByEntityId.cpp
    mapGetContextEntityAttributes.cpp
    mapGetEntityByIdAttributeByName.cpp
    mapGetContextEntityTypes.cpp
    mapGetIndividualContextEntity.cpp
    mapPutIndividualContextEntity.cpp
    mapPostIndividualContextEntity.cpp
    mapDeleteIndividualContextEntity.cpp
    mapGetIndividualContextEntityAttributes.cpp
    mapPutIndividualContextEntityAttributes.cpp
    mapPostIndividualContextEntityAttributes.cpp
    mapDeleteIndividualContextEntityAttributes.cpp
    mapGetIndividualContextEntityAttribute.cpp
    mapPutIndividualContextEntityAttribute.cpp
    mapPostIndividualContextEntityAttribute.cpp
    mapDeleteIndividualContextEntityAttribute.cpp
)

SET (HEADERS
    mapGetContextEntitiesByEntityId.h
    mapGetContextEntityAttributes.h
    mapGetEntityByIdAttributeByName.h
    mapGetContextEntityTypes.h
    mapGetIndividualContextEntity.h
    mapPutIndividualContextEntity.h
    mapPostIndividualContextEntity.h
    mapDeleteIndividualContextEntity.h
    mapGetIndividualContextEntityAttributes.h
    mapPutIndividualContextEntityAttributes.h
    mapPostIndividualContextEntityAttributes.h
    mapDeleteIndividualContextEntityAttributes.h
    mapGetIndividualContextEntityAttribute.h
    mapPutIndividualContextEntityAttribute.h
    mapPostIndividualContextEntityAttribute.h
    mapDeleteIndividualContextEntityAttribute.h
)



# Include directories
# -----------------------------------------------------------------
include_directories("${PROJECT_SOURCE_DIR}/src/lib")


# Library declaration
# -----------------------------------------------------------------
ADD_LIBRARY(convenienceMap STATIC ${SOURCES} ${HEADERS})
