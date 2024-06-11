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

CMAKE_MINIMUM_REQUIRED(VERSION 3.0)

SET (SOURCES
    NotifyContextRequest.cpp
    NotifyContextResponse.cpp
    QueryContextRequest.cpp
    QueryContextResponse.cpp
    SubscribeContextRequest.cpp
    SubscribeContextResponse.cpp
    UnsubscribeContextRequest.cpp
    UnsubscribeContextResponse.cpp
    UpdateContextRequest.cpp
    UpdateContextResponse.cpp
    UpdateContextSubscriptionRequest.cpp
    UpdateContextSubscriptionResponse.cpp
)

SET (HEADERS
    NotifyContextRequest.h
    NotifyContextResponse.h
    QueryContextRequest.h
    QueryContextResponse.h
    SubscribeContextRequest.h
    SubscribeContextResponse.h
    UnsubscribeContextRequest.h
    UnsubscribeContextResponse.h
    UpdateContextRequest.h
    UpdateContextResponse.h
    UpdateContextSubscriptionRequest.h
    UpdateContextSubscriptionResponse.h
)



# Include directories
# -----------------------------------------------------------------
include_directories("${PROJECT_SOURCE_DIR}/src/lib")


# Library declaration
# -----------------------------------------------------------------
ADD_LIBRARY(ngsi10 STATIC ${SOURCES} ${HEADERS})
