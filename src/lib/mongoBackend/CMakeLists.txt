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

CMAKE_MINIMUM_REQUIRED(VERSION 3.5)

SET (SOURCES
    MongoGlobal.cpp
    MongoCommonUpdate.cpp
    MongoCommonSubscription.cpp
    mongoUpdateContext.cpp
    mongoQueryContext.cpp
    mongoUnsubscribeContext.cpp
    mongoNotifyContext.cpp
    mongoQueryTypes.cpp
    mongoCreateSubscription.cpp
    mongoUpdateSubscription.cpp
    TriggeredSubscription.cpp
    mongoGetSubscriptions.cpp
    mongoRegistrationGet.cpp
    mongoRegistrationCreate.cpp
    mongoRegistrationDelete.cpp
    mongoSubCache.cpp
    compoundResponses.cpp
    location.cpp
    compoundValueBson.cpp
    dateExpiration.cpp
)

SET (HEADERS
    dbConstants.h
    MongoGlobal.h
    MongoCommonUpdate.h
    MongoCommonSubscription.h
    mongoUpdateContext.h
    mongoQueryContext.h
    mongoUnsubscribeContext.h
    mongoNotifyContext.h
    mongoQueryTypes.h
    mongoCreateSubscription.h
    mongoUpdateSubscription.h
    TriggeredSubscription.h
    mongoGetSubscriptions.h
    mongoRegistrationGet.h
    mongoRegistrationCreate.h
    mongoRegistrationDelete.h
    mongoSubCache.h
    dbFieldEncoding.h
    compoundResponses.h
    location.h
    compoundValueBson.h
    dateExpiration.h
)



# Include directories
# -----------------------------------------------------------------
include_directories("${PROJECT_SOURCE_DIR}/src/lib")


# Library declaration
# -----------------------------------------------------------------
ADD_LIBRARY(mongoBackend STATIC ${SOURCES} ${HEADERS})
