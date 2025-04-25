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
versionTreat.cpp
exitTreat.cpp
leakTreat.cpp
logTraceTreat.cpp
postUpdateContext.cpp
postQueryContext.cpp
badVerbPostOnly.cpp
badVerbGetPostOnly.cpp
badVerbGetOnly.cpp
badVerbGetDeleteOnly.cpp
badVerbPutDeleteOnly.cpp
badVerbGetPutDeleteOnly.cpp
badVerbPutOnly.cpp
badRequest.cpp
postNotifyContext.cpp
statisticsTreat.cpp
optionsVersionRequest.cpp
)

SET (HEADERS
versionTreat.h
exitTreat.h
logTraceTreat.h
postUpdateContext.h
postQueryContext.h
badVerbPostOnly.h
badVerbGetPostOnly.h
badVerbGetOnly.h
badVerbGetDeleteOnly.h
badVerbPutDeleteOnly.h
badVerbGetPutDeleteOnly.h
badVerbPutOnly.h
badRequest.h
postNotifyContext.h
statisticsTreat.h
optionsVersionRequest.h
)



# Include directories
# -----------------------------------------------------------------
include_directories("${PROJECT_SOURCE_DIR}/src/lib")

# Library declaration
# -----------------------------------------------------------------
ADD_LIBRARY(serviceRoutines STATIC ${SOURCES} ${HEADERS})
