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
    xmlRegisterContextRequest.cpp
    xmlDiscoverContextAvailabilityRequest.cpp
    xmlSubscribeContextAvailabilityRequest.cpp
    xmlUnsubscribeContextAvailabilityRequest.cpp
    xmlUpdateContextAvailabilitySubscriptionRequest.cpp
    xmlNotifyContextAvailabilityRequest.cpp

    xmlQueryContextRequest.cpp
    xmlQueryContextResponse.cpp
    xmlSubscribeContextRequest.cpp
    xmlUnsubscribeContextRequest.cpp
    xmlUpdateContextRequest.cpp
    xmlUpdateContextResponse.cpp
    xmlUpdateContextSubscriptionRequest.cpp
    xmlNotifyContextRequest.cpp

    xmlRegisterProviderRequest.cpp
    xmlUpdateContextElementRequest.cpp
    xmlAppendContextElementRequest.cpp
    xmlUpdateContextAttributeRequest.cpp

    xmlRegisterContextResponse.cpp

    xmlRequest.cpp
    xmlParse.cpp
)

SET (HEADERS
    xmlRegisterContextRequest.h
    xmlDiscoverContextAvailabilityRequest.h
    xmlSubscribeContextAvailabilityRequest.h
    xmlUnsubscribeContextAvailabilityRequest.h
    xmlUpdateContextAvailabilitySubscriptionRequest.h
    xmlNotifyContextAvailabilityRequest.h

    xmlQueryContextRequest.h
    xmlQueryContextResponse.h
    xmlSubscribeContextRequest.h
    xmlUnsubscribeContextRequest.h
    xmlUpdateContextRequest.h
    xmlUpdateContextResponse.h
    xmlUpdateContextSubscriptionRequest.h
    xmlNotifyContextRequest.h

    xmlRegisterProviderRequest.h
    xmlUpdateContextElementRequest.h
    xmlAppendContextElementRequest.h
    xmlUpdateContextAttributeRequest.h

    xmlRegisterContextResponse.h

    xmlRequest.h
    xmlParse.h
)



# Include directories
# -----------------------------------------------------------------
include_directories("${PROJECT_SOURCE_DIR}/src/lib")
include_directories("${RAPIDXML_INCLUDE}")


# Library declaration
# -----------------------------------------------------------------
ADD_LIBRARY(xmlParse STATIC ${SOURCES} ${HEADERS})
