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

# find_package() for mongo driver doesn't work with cmake 2
cmake_minimum_required(VERSION 3.5)

# set C++14 standard
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

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

# FIXME #3775 currently the only place in which std::auto_ptr<std::auto_ptr> is being used
# is the code in commonMocks.h. Thus. probably this can be removed and #2979 definitively solved.
#
# FIXME P4: https://github.com/telefonicaid/fiware-orion/issues/2979
#   'template class std::auto_ptr' is deprecated in gcc 6.3.0 of Ubuntu 17.04, so -Werror cannot be used alone, at least not for now.
#
#   The *real* fix would be to rewrite the part of the code producing these warnings, but for now,
#   disabling the -Werror flag makes the problem go away (warnings will still be found and give errors in other distros).
#
#   One example of this warning (there are many warnings, but as in header files, just a few places):
#
#   mongoBackend/safeMongo.h:
#   ----------------------------------------------------------------
#     extern bool nextSafeOrError
#     (
#       const std::auto_ptr<mongo::DBClientCursor>&  cursor,
#       ...
#   ----------------------------------------------------------------
#
# A grep of 'std::auto_ptr<' in the header files show 7 hits, and 13 hits in cpp files
#
# In PR https://github.com/telefonicaid/fiware-orion/pull/3069, a workaround for Ubuntu is implemented.
# This workaround consists of putting back the -Werror flag and instead solving the problem by
# adding another flag: -Wno-deprecated-declarations
#
if (CMAKE_CXX_COMPILER_VERSION VERSION_LESS 6.3)
  message ("gcc version less than 6.3, compiling without -Wno-deprecated-declarations")
  set (CMAKE_CXX_FLAGS "-Wall -Wno-unknown-pragmas -D_LARGEFILE64_SOURCE -D_GNU_SOURCE -Werror -fno-var-tracking-assignments")
elseif(CMAKE_CXX_COMPILER_VERSION VERSION_LESS 8.3)
  # Formerly this was a condition based in distribution, set for Ubuntu_17.*, Ubuntu_18.* and Debian_9.*
  message("gcc version between 6.3 and 8.3, compiling with -Wno-deprecated-declarations")
  set (CMAKE_CXX_FLAGS "-Wall -Wno-unknown-pragmas -D_LARGEFILE64_SOURCE -D_GNU_SOURCE -Werror -Wno-deprecated-declarations -fno-var-tracking-assignments")
else()
  message("gcc version higher than 8.3, compiling with -Wno-deprecated-declarations -Wno-format-truncation -Wno-stringop-truncation")
  set (CMAKE_CXX_FLAGS "-Wall -Wno-unknown-pragmas -D_LARGEFILE64_SOURCE -D_GNU_SOURCE -Werror -Wno-deprecated-declarations -fno-var-tracking-assignments -Wno-format-truncation -Wno-stringop-truncation")
endif()

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
    # OLD:
    #   Note that stack protection is activated only in debug compilation.
    #   This is due that this directive makes the compiler introduce code at the beginning/end of each functions,
    #   so it has a pretty big impact on performance
    #
    #   Our unit tests don't pass the compilation with "stack protection", so we turn it off using
    #   remove_definitions().
    #
    #add_definitions(-fstack-check -fstack-protector)
    add_definitions(-fstack-protector)
    #
    # NEW:
    #   The option -fstack-check makes valgrind report false positives (millions) in CentOS 6.
    #   This makes the output impossible to use and we have decided to remove these two options (both of them).
    #   The cause seems to be a bug in the compiler (gcc 4.4.7), fixed in some moment between that version 
    #   and gcc 4.7.2 (see https://stackoverflow.com/questions/44758939/unexpected-valgrind-invalid-write-when-fstack-check-is-used
    #   for more detail). However, we don't want to use a compiler that is not meant for CentOS 6,
    #   which is the official distro for the context broker, so, we simply remove this option.
    #   They may be reinstated once Orion context broker updates the official distro to be CentOS 7 (which uses gcc 4.8.5)
    #  

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

# Enables some some #ifdef in the code for compiling in old system
# FIXME: cleanup OLD_SSL_VERSION_FORMAT stuff after consolidating the change to Debian 12+
IF((${DISTRO} MATCHES "Debian_11.*") OR (${DISTRO} MATCHES "Alpine_3.16.*"))
  ADD_DEFINITIONS(-DOLD_SSL_VERSION_FORMAT)
ENDIF()

#
# Libraries
#

SET (ORION_LIBS
    common
    rest          # verbName(Verb) from setExtendedHttpInfo@MongoCommonSubscription.cpp.o; jsonRequestTreat from payloadParse@RestService.cpp.o;
    serviceRoutinesV2
    ngsiNotify
    jsonParseV2
    rest          # this is repeated for linking str2Verb from parseSubscription@parseSubscription.cpp.o;
                  # verbName(Verb) from setExtendedHttpInfo@MongoCommonSubscription.cpp.o
    mongoBackend
    ngsi
    cache
    mongoBackend  # repeated (probably due to some circular dependency between mongoBackend and ngsi)
    mongoDriver
    parse
    apiTypesV2
    orionTypes
    common
    alarmMgr
    metricsMgr
    expressions
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

# See http://mongoc.org/libmongoc/current/tutorial.html#cmake
# This find_package() command provides the mongo::mongoc_static used in
# SET for common static libs. We use 1.29.0 as reference version.
find_package (mongoc-1.0 1.29.0 EXACT)

# Is cjexl lib available?
find_library (HAVE_CJEXL cjexl PATHS /usr/lib /usr/lib64 /usr/local/lib64 /usr/local/lib)
if (HAVE_CJEXL)
    message("Using cjexl")
    SET (COMMON_STATIC_LIBS
        microhttpd.a
        mosquitto.a
        ${HAVE_CJEXL}
        mongo::mongoc_static
    )
else (HAVE_CJEXL)
    message("Not using cjexl")
    add_definitions(-DEXPR_BASIC)
    SET (COMMON_STATIC_LIBS
        microhttpd.a
        mosquitto.a
        mongo::mongoc_static
    )
endif (HAVE_CJEXL)

# Static libs common to contextBroker and unitTest binaries


SET (DYNAMIC_LIBS
    curl
    gnutls
    pthread
    gcrypt
    ssl
    uuid
    crypto
    sasl2
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

# Needed for the new C driver
include_directories("/usr/local/include/libmongoc-1.0")
include_directories("/usr/local/include/libbson-1.0")


#
# Library directories
#
MESSAGE("Building for ${BUILD_ARCH}")
if ("${BUILD_ARCH}" STREQUAL "x86_64")
    link_directories("/usr/lib64")
elseif ("${BUILD_ARCH}" STREQUAL "aarch64")
    link_directories("/usr/lib64")
else (NOT "${BUILD_ARCH}" STREQUAL "x86_64")
    link_directories("/usr/lib")
endif ("${BUILD_ARCH}" STREQUAL "x86_64")
link_directories("/usr/lib/x86_64-linux-gnu")
#
# Enabling test harness
#
if (COVERAGE AND NOT UNIT_TEST)
   ENABLE_TESTING()
   INCLUDE(Dart)
   SET (CMAKE_TEST_TIMEOUT 60)
   SET (DART_TIMEOUT 60)
   ADD_SUBDIRECTORY(test/functionalTest/cases)
endif (COVERAGE AND NOT UNIT_TEST)

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
  ADD_SUBDIRECTORY(src/lib/serviceRoutinesV2)
  ADD_SUBDIRECTORY(src/lib/ngsiNotify)
  ADD_SUBDIRECTORY(src/lib/apiTypesV2)
  ADD_SUBDIRECTORY(src/lib/parse)
  ADD_SUBDIRECTORY(src/lib/jsonParseV2)
  ADD_SUBDIRECTORY(src/lib/rest)
  ADD_SUBDIRECTORY(src/lib/mongoBackend)
  ADD_SUBDIRECTORY(src/lib/mongoDriver)
  ADD_SUBDIRECTORY(src/lib/cache)
  ADD_SUBDIRECTORY(src/lib/alarmMgr)
  ADD_SUBDIRECTORY(src/lib/metricsMgr)
  ADD_SUBDIRECTORY(src/lib/expressions)
  ADD_SUBDIRECTORY(src/lib/logSummary)
  ADD_SUBDIRECTORY(src/lib/mqtt)
  ADD_SUBDIRECTORY(src/app/contextBroker)
else ()
    MESSAGE("cmake: NOT OK")
endif (error EQUAL 0)
