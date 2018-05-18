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

#
# Project name and basic prerequisities
#
PROJECT(contextBroker)
cmake_minimum_required(VERSION 2.6)

#
# DEBUG or RELEASE build ?
#

if (NOT CMAKE_BUILD_TYPE)
    MESSAGE("CMAKE_BUILD_TYPE not set - setting it to RELEASE")
    SET (CMAKE_BUILD_TYPE "RELEASE")
else (NOT CMAKE_BUILD_TYPE)
    MESSAGE("CMAKE_BUILD_TYPE set to ${CMAKE_BUILD_TYPE}")
endif (NOT CMAKE_BUILD_TYPE)

set(error 0)

execute_process(COMMAND            echo $USER
                COMMAND            tr 'a-z' 'A-Z'
                OUTPUT_VARIABLE    USER)


execute_process(COMMAND            sh ../scripts/build/osDistro.sh
                OUTPUT_VARIABLE    DISTRO)

# TRACES: To remove LM_T
#           [ and 15 log macros more, of which only LM_T is used by the broker.
#             The important ones: LM_W, LM_E, LM_X, LM_I are all kept as is ]
#         comment the following line
add_definitions(-DLM_ON)

add_definitions(-fPIC)

# Baseline compiler flags, any change here will affect all build types
#set (CMAKE_CXX_FLAGS "-Wall -Wno-unknown-pragmas -D_LARGEFILE64_SOURCE -D_GNU_SOURCE -Werror")
set (CMAKE_CXX_FLAGS "-Wall -Wno-unknown-pragmas -D_LARGEFILE64_SOURCE -D_GNU_SOURCE -Werror -fno-var-tracking-assignments")

#
# Platform checks
# 
MESSAGE( "Compiling in system ${CMAKE_SYSTEM}")

IF (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    ADD_DEFINITIONS(-DLINUX)
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DLINUX")
ENDIF (${CMAKE_SYSTEM_NAME} MATCHES "Linux")


MESSAGE("CMAKE_BUILD_TYPE: '${CMAKE_BUILD_TYPE}'")

if (${CMAKE_BUILD_TYPE} STREQUAL DEBUG)

    MESSAGE("cmake: DEBUG compilation")
    add_definitions(-DDEBUG -DDEBUG_$ENV{USER} -DDEBUG_${USER})

    #
    # Note that stack protection is activated only in debug compilation.
    # This is due that this directive makes the compiler introduce code at the beginning/end of each functions,
    # so it has a pretty big impact on performance
    #
    # Our unit tests don't pass the compilation with "stack protection", so we turn it off using
    # remove_definitions().
    #
    add_definitions(-fstack-check -fstack-protector)
    if (COVERAGE)
        MESSAGE("cmake: Coverage compilation")
        # LM_T macros introduce condition branchs which are not never traversed. Removing LM_T macros doesn't have 
        # any impact in line coverage but removes "noise" in condition coverage, thus making coverage reports more useful
        remove_definitions(-DLM_ON)
        # Removing LM_T macros make that some variables get unused, thus -Wno-unused-variable have to be used
        set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-variable -fprofile-arcs -ftest-coverage")
        set (CMAKE_EXE_LINKER_FLAGS "-fprofile-arcs -ftest-coverage")
    endif (COVERAGE)

    if (UNIT_TEST)
        add_definitions( -DUNIT_TEST )
    endif (UNIT_TEST)

else (${CMAKE_BUILD_TYPE} STREQUAL DEBUG)

    MESSAGE("cmake: RELEASE compilation")

    if (STRICT)
        MESSAGE("cmake: Strict compilation")
        set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wextra")
    endif (STRICT)

endif (${CMAKE_BUILD_TYPE} STREQUAL DEBUG)

# -Wunused-but-set-variable is available from gcc 4.6.0 but can be set in 4.5.x
# OS X comes with gcc 4.2.1 (10.7) and does not accept this flag
# 
# no-unused-but-set-variable (commented, not active):
#   if the following piece of 'compiler directives' is uncommented, we no longer get warnings for
#   variables that are defined but not used.
#   For now we want these warnings as we desire as strict compilaton as possible.
#
#if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
#    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-but-set-variable")
#endif (${CMAKE_SYSTEM_NAME} MATCHES "Linux")

#
# Libraries
#

SET (ORION_LIBS
    common
    serviceRoutines
    serviceRoutinesV2
    ngsiNotify
    rest          # verbName(Verb) from setExtendedHttpInfo@MongoCommonSubscription.cpp.o; jsonRequestTreat from payloadParse@RestService.cpp.o;
    jsonParse
    jsonParseV2
    rest          # this is repeated for linking str2Verb from parseSubscription@parseSubscription.cpp.o;
                  # verbName(Verb) from setExtendedHttpInfo@MongoCommonSubscription.cpp.o
    convenience
    ngsi9
    ngsi10
    ngsi
    cache
    mongoBackend
    parse
    apiTypesV2
    orionTypes
    common
    alarmMgr
    metricsMgr
    logSummary
    lm
    pa
    mqtt
)

SET (BOOST
    boost_thread
    boost_filesystem
    boost_system
    boost_regex
)

SET (BOOST_MT
    boost_thread-mt
    boost_filesystem-mt
    boost_system-mt
    boost_regex-mt
)

# Static libs common to contextBroker and unitTest binaries
SET (COMMON_STATIC_LIBS
    microhttpd.a
    mongoclient.a
)

SET (DYNAMIC_LIBS
    curl
    gnutls
    pthread
    gcrypt
    ssl
    uuid
    mosquitto
)

#
# Enabling testing
#
if (UNIT_TEST)
   ENABLE_TESTING()
   INCLUDE(Dart)
   SET (CMAKE_TEST_TIMEOUT 60)
   SET (DART_TIMEOUT 60)
   ADD_SUBDIRECTORY(test)
endif (UNIT_TEST)


#
# Common include
#
include_directories("/usr/include")

#
# Library directories
#
MESSAGE("Building for ${BUILD_ARCH}")
if ("${BUILD_ARCH}" STREQUAL "x86_64")
    link_directories("/usr/lib64")
else (NOT "${BUILD_ARCH}" STREQUAL "x86_64")
    link_directories("/usr/lib")
endif ("${BUILD_ARCH}" STREQUAL "x86_64")
link_directories("/usr/lib/x86_64-linux-gnu")
#
# Enabling test harness
#
if (UNIT_TEST)
   ENABLE_TESTING()
   INCLUDE(Dart)
   SET (CMAKE_TEST_TIMEOUT 60)
   SET (DART_TIMEOUT 60)
   #ADD_SUBDIRECTORY(test/harness)
endif (UNIT_TEST)

#
# Continuing if OK
#
if (error EQUAL 0)
  MESSAGE("cmake: OK")
  ADD_SUBDIRECTORY(src/lib/logMsg)
  ADD_SUBDIRECTORY(src/lib/parseArgs)
  ADD_SUBDIRECTORY(src/lib/common)
  ADD_SUBDIRECTORY(src/lib/orionTypes)
  ADD_SUBDIRECTORY(src/lib/ngsi)
  ADD_SUBDIRECTORY(src/lib/serviceRoutines)
  ADD_SUBDIRECTORY(src/lib/serviceRoutinesV2)
  ADD_SUBDIRECTORY(src/lib/convenience)
  ADD_SUBDIRECTORY(src/lib/ngsi9)
  ADD_SUBDIRECTORY(src/lib/ngsi10)
  ADD_SUBDIRECTORY(src/lib/ngsiNotify)
  ADD_SUBDIRECTORY(src/lib/apiTypesV2)
  ADD_SUBDIRECTORY(src/lib/parse)
  ADD_SUBDIRECTORY(src/lib/jsonParse)
  ADD_SUBDIRECTORY(src/lib/jsonParseV2)
  ADD_SUBDIRECTORY(src/lib/rest)
  ADD_SUBDIRECTORY(src/lib/mongoBackend)
  ADD_SUBDIRECTORY(src/lib/cache)
  ADD_SUBDIRECTORY(src/lib/alarmMgr)
  ADD_SUBDIRECTORY(src/lib/metricsMgr)
  ADD_SUBDIRECTORY(src/lib/logSummary)
  ADD_SUBDIRECTORY(src/lib/mqtt)
  ADD_SUBDIRECTORY(src/app/contextBroker)
else ()
    MESSAGE("cmake: NOT OK")
endif (error EQUAL 0)
