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
    jsonRequest.cpp
    jsonParse.cpp

    jsonRegisterContextRequest.cpp
    jsonDiscoverContextAvailabilityRequest.cpp 

    jsonQueryContextRequest.cpp
    jsonQueryContextResponse.cpp
    jsonUpdateContextRequest.cpp
    jsonUpdateContextResponse.cpp
    jsonSubscribeContextRequest.cpp
    jsonNotifyContextRequest.cpp
    jsonUnsubscribeContextRequest.cpp
    jsonUpdateContextSubscriptionRequest.cpp
    
    jsonRegisterProviderRequest.cpp
    jsonUpdateContextElementRequest.cpp
    jsonAppendContextElementRequest.cpp
    jsonUpdateContextAttributeRequest.cpp
)

SET (HEADERS
    jsonRequest.h
    jsonParse.h
    JsonNode.h

    jsonRegisterContextRequest.h
    jsonDiscoverContextAvailabilityRequest.h

    jsonQueryContextRequest.h
    jsonQueryContextResponse.h
    jsonUpdateContextRequest.h
    jsonUpdateContextResponse.h
    jsonSubscribeContextRequest.h
    jsonNotifyContextRequest.h
    jsonUnsubscribeContextRequest.h
    jsonUpdateContextSubscriptionRequest.h
    
    jsonRegisterProviderRequest.h
    jsonUpdateContextElementRequest.h
    jsonAppendContextElementRequest.h
    jsonUpdateContextAttributeRequest.h
)



# Include directories
# -----------------------------------------------------------------
include_directories("${PROJECT_SOURCE_DIR}/src/lib")


# Library declaration
# -----------------------------------------------------------------
ADD_LIBRARY(jsonParse STATIC ${SOURCES} ${HEADERS})
