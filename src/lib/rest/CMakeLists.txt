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
# iot_support at tid dot es

CMAKE_MINIMUM_REQUIRED(VERSION 2.6)

SET (SOURCES
    rest.cpp
    restReply.cpp
    RestService.cpp
    Verb.cpp
    curlSem.cpp
    httpRequestSend.cpp
    orionLogReply.cpp
    OrionError.cpp
    HttpStatusCode.cpp
    ConnectionInfo.cpp
    StringFilter.cpp
    HttpHeaders.cpp
    restServiceLookup.cpp
)

SET (HEADERS
    ConnectionInfo.h
    HttpHeaders.h
    mhd.h
    rest.h
    restReply.h
    RestService.h
    Verb.h
    curlSem.h
    httpRequestSend.h
    orionLogReply.h
    OrionError.h
    HttpStatusCode.h
    StringFilter.h
    restServiceLookup.h
)



# Include directories
# -----------------------------------------------------------------
include_directories("${PROJECT_SOURCE_DIR}/src/lib")


# Library declaration
# -----------------------------------------------------------------
ADD_LIBRARY(rest STATIC ${SOURCES} ${HEADERS})
