# Copyright 2014 Telefonica Investigacion y Desarrollo, S.A.U
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
    CompoundValueNode.cpp
    compoundValue.cpp
    nullTreat.cpp
    forbiddenChars.cpp
    jsonRequestTreat.cpp
    parseEntity.cpp
    parseContextAttribute.cpp
    parseContextAttributeCompoundValue.cpp
    parseMetadata.cpp
    jsonParseTypeNames.cpp
)

SET (HEADERS
    CompoundValueNode.h
    compoundValue.h
    nullTreat.h
    forbiddenChars.h
    jsonRequestTreat.h
    parseEntity.h
    parseContextAttribute.h
    parseContextAttributeCompoundValue.cpp
    parseMetadata.h
    jsonParseTypeNames.h
)



# Include directories
# -----------------------------------------------------------------
include_directories("${PROJECT_SOURCE_DIR}/src/lib")
include_directories("/opt/rapidjson/rapidjson-1.0.2/include")

# Library declaration
# -----------------------------------------------------------------
ADD_LIBRARY(parse STATIC ${SOURCES} ${HEADERS})
