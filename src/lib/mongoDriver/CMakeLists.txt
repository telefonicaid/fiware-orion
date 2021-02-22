# Copyright 2020 Telefonica Investigacion y Desarrollo, S.A.U
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
    BSONTypes.cpp
    BSONObj.cpp
    BSONObjBuilder.cpp
    BSONArray.cpp
    BSONArrayBuilder.cpp
    BSONElement.cpp
    BSONDate.cpp
    OID.cpp
    DBConnection.cpp
    DBCursor.cpp
    safeMongo.cpp
    connectionOperations.cpp
    mongoConnectionPool.cpp
)

SET (HEADERS
    BSONTypes.h
    BSONObj.h
    BSONObjBuilder.h
    BSONArray.h
    BSONArrayBuilder.h
    BSONElement.h
    BSONDate.h
    OID.h
    DBConnection.h
    DBCursor.h
    WriteConcern.h
    safeMongo.h
    connectionOperations.h
    mongoConnectionPool.h
)



# Include directories
# -----------------------------------------------------------------
include_directories("${PROJECT_SOURCE_DIR}/src/lib")


# Library declaration
# -----------------------------------------------------------------
ADD_LIBRARY(mongoDriver STATIC ${SOURCES} ${HEADERS})
