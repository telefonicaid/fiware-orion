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

# ---------------------------------------------------------
# External dependency checks
# ---------------------------------------------------------

FIND_LIBRARY (HAVE_GTEST gtest PATHS /usr/lib /usr/lib64 /usr/local/lib64 /usr/local/lib)
if (HAVE_GTEST)
    MESSAGE("Using ${HAVE_GTEST} for Google Test")
else (HAVE_GTEST)
    MESSAGE("Not using the Google Test library")
endif (HAVE_GTEST)

find_library(GTESTMAIN_LIBRARY gtest_main
             PATHS /usr/lib64 /usr/local/lib64 /usr/local/lib)


find_path(GTEST_INCLUDE_DIR gtest/gtest.h
          PATHS /usr/includes 
          PATH_SUFFIXES gtest)

FIND_LIBRARY (HAVE_GMOCK gmock PATHS /usr/lib /usr/lib64 /usr/local/lib64 /usr/local/lib)
if (HAVE_GMOCK)
    MESSAGE("Using ${HAVE_GMOCK} for Google Mock")
else (HAVE_GMOCK)
    MESSAGE("Not using the Google Mock library")
endif (HAVE_GMOCK)

find_library(GMOCKMAIN_LIBRARY gmock_main
             PATHS /usr/lib64 /usr/local/lib64 /usr/local/lib)

find_path(GMOCK_INCLUDE_DIR gmock/gmock.h
          PATHS /usr/includes
          PATH_SUFFIXES gmock)


#
# Stack protection is activated only in debug compilation.
# However, our unit tests don't pass the compilation with "stack protection", so we turn it off using
# remove_definitions().
#
remove_definitions(-fstack-check -fstack-protector)

SET (SOURCES
    main_UnitTest.cpp
    testDataFromFile.cpp
    testInit.cpp
    unittest.cpp

    serviceRoutines/badVerbGetOnly_test.cpp
    serviceRoutines/badVerbPostOnly_test.cpp
    serviceRoutines/badVerbAllFour_test.cpp
    serviceRoutines/badVerbGetPostOnly_test.cpp
    serviceRoutines/badVerbGetPostDeleteOnly_test.cpp
    serviceRoutines/badVerbGetDeleteOnly_test.cpp
    serviceRoutines/badVerbGetPutDeleteOnly_test.cpp
    serviceRoutines/putIndividualContextEntityAttribute_test.cpp
    serviceRoutines/putSubscriptionConvOp_test.cpp
    serviceRoutines/versionTreat_test.cpp
    serviceRoutines/exitTreat_test.cpp
    serviceRoutines/statisticsTreat_test.cpp

    convenience/Convenience_test.cpp
    convenience/AppendContextElementRequest_test.cpp
    convenience/AppendContextElementResponse_test.cpp
    convenience/ContextAttributeResponse_test.cpp
    convenience/ContextAttributeResponseVector_test.cpp
    convenience/UpdateContextAttributeRequest_test.cpp
    convenience/UpdateContextElementRequest_test.cpp
    convenience/UpdateContextElementResponse_test.cpp
    convenience/RegisterProviderRequest_test.cpp
    common/commonFormat_test.cpp
    common/commonGlobals_test.cpp
    common/commonString_test.cpp
    common/commonTag_test.cpp
    common/commonSem_test.cpp
    # FIXME P3: this test is aligned with the old version of the statistics logic for NGSIv1
    # it should be moved to NGSIv2 or, alternatively, removed the file at the end
    #common/commonStatistics_test.cpp
    common/commonWsStrip_test.cpp
    common/commonMacroSubstitute_test.cpp

    ngsi9/RegisterContextRequest_test.cpp
    ngsi9/RegisterContextResponse_test.cpp
    ngsi9/DiscoverContextAvailabilityRequest_test.cpp
    ngsi9/DiscoverContextAvailabilityResponse_test.cpp

    ngsi10/QueryContextRequest_test.cpp
    ngsi10/QueryContextResponse_test.cpp
    ngsi10/NotifyContextRequest_test.cpp
    ngsi10/SubscribeContextRequest_test.cpp
    ngsi10/SubscribeContextResponse_test.cpp
    ngsi10/UnsubscribeContextRequest_test.cpp
    ngsi10/UnsubscribeContextResponse_test.cpp
    ngsi10/UpdateContextRequest_test.cpp
    ngsi10/UpdateContextResponse_test.cpp
    ngsi10/UpdateContextSubscriptionRequest_test.cpp
    ngsi10/UpdateContextSubscriptionResponse_test.cpp

    ngsi/ContextAttribute_test.cpp
    ngsi/NotifyCondition_test.cpp
    ngsi/AttributeList_test.cpp
    ngsi/ConditionValueList_test.cpp
    ngsi/ContextAttributeVector_test.cpp
    ngsi/ContextElementResponse_test.cpp
    ngsi/ContextElementResponseVector_test.cpp
    ngsi/ContextRegistrationAttribute_test.cpp
    ngsi/ContextRegistrationAttributeVector_test.cpp
    ngsi/ContextRegistrationVector_test.cpp
    ngsi/ProvidingApplication_test.cpp
    ngsi/Duration_test.cpp
    ngsi/AttributeExpression_test.cpp
    ngsi/ContextRegistrationResponse_test.cpp
    ngsi/ContextRegistrationResponseVector_test.cpp
    ngsi/EntityId_test.cpp
    ngsi/Metadata_test.cpp
    ngsi/MetadataVector_test.cpp
    ngsi/NotifyConditionVector_test.cpp
    ngsi/Originator_test.cpp
    ngsi/Reference_test.cpp
    ngsi/Restriction_test.cpp
    ngsi/Request_test.cpp
    ngsi/RestrictionString_test.cpp
    ngsi/Scope_test.cpp
    ngsi/ScopeVector_test.cpp
    ngsi/StatusCode_test.cpp
    ngsi/SubscribeError_test.cpp
    ngsi/SubscriptionId_test.cpp
    ngsi/Throttling_test.cpp

    orionTypes/EntityType_test.cpp
    orionTypes/EntityTypeResponse_test.cpp
    orionTypes/EntityTypeVector_test.cpp
    orionTypes/EntityTypeVectorResponse_test.cpp

    apiTypesV2/Entities_test.cpp
    apiTypesV2/Entity_test.cpp  
    apiTypesV2/EntityVector_test.cpp

    # FIXME #3775: pending on mongo unit test re-enabling
    #mongoBackend/mongoDiscoverContextAvailability_test.cpp
    #mongoBackend/mongoQueryContext_test.cpp
    #mongoBackend/mongoQueryContext_filters_test.cpp
    #mongoBackend/mongoQueryContextCompoundValues_test.cpp
    #mongoBackend/mongoQueryContextGeo_test.cpp
    #mongoBackend/mongoRegisterContext_test.cpp
    #mongoBackend/mongoRegisterContext_update_test.cpp
    #mongoBackend/mongoSubscribeContext_test.cpp
    #mongoBackend/mongoUnsubscribeContext_test.cpp
    #mongoBackend/mongoUpdateContextSubscription_test.cpp
    #mongoBackend/mongoUpdateContext_test.cpp
    #mongoBackend/mongoUpdateContext_2_test.cpp
    #mongoBackend/mongoUpdateContextGeo_test.cpp
    #mongoBackend/mongoUpdateContext_withOnchangeSubscriptions_test.cpp
    #mongoBackend/mongoUpdateContext_withOnchangeSubscriptionsNoCache_test.cpp
    #mongoBackend/mongoUpdateContextCompoundValues_test.cpp
    #mongoBackend/mongoNotifyContext_test.cpp
    #mongoBackend/mongoContextProvidersQuery_test.cpp
    #mongoBackend/mongoContextProvidersUpdate_test.cpp
    #mongoBackend/mongoQueryTypes_test.cpp
    #mongoBackend/mongoQueryContextFilterExistEntity_test.cpp
    #mongoBackend/mongoGetSubscriptions_test.cpp
    #mongoBackend/mongoCreateSubscription_test.cpp

    parse/CompoundValueNode_test.cpp
    parse/compoundValue_test.cpp
    parse/nullTreat_test.cpp
    jsonParse/jsonRequest_test.cpp

    rest/OrionError_test.cpp
    rest/Verb_test.cpp
    rest/restReply_test.cpp
    rest/RestService_test.cpp
    rest/rest_test.cpp
)

SET (HEADERS
    ${GTEST_INCLUDE_DIR}
    ${GMOCK_INCLUDE_DIR}
)

SET (STATIC_LIBS
    ${HAVE_GTEST}
    ${GTESTMAIN_LIBRARY}
    ${HAVE_GMOCK}
    ${GMOCKMAIN_LIBRARY}
    ${ORION_LIBS}
    ${COMMON_STATIC_LIBS}
)

# Include directories
# ------------------------------------------------------------
include_directories("${PROJECT_SOURCE_DIR}")
include_directories("${PROJECT_SOURCE_DIR}/src/lib")
include_directories("${PROJECT_SOURCE_DIR}/src/app")
include_directories("${PROJECT_SOURCE_DIR}/test/unittests")
include_directories("${PROJECT_SOURCE_DIR}/test")

# Needed for the new C driver
# FIXME: why do we need this declared here if they are also declared
# in the main CMakeFiles.txt?
include_directories("/usr/local/include/libmongoc-1.0")
include_directories("/usr/local/include/libbson-1.0")

# Lib directories
# ------------------------------------------------------------
link_directories("/usr/local/lib/")
link_directories("/usr/lib64/")
link_directories("/usr/lib/x86_64-linux-gnu")



# flags
# ------------------------------------------------------------
add_definitions(-DUNIT_TEST)



# Executable declaration
# ------------------------------------------------------------

ADD_EXECUTABLE(unitTest ${SOURCES} ${HEADERS})

MESSAGE("unitTest distro: '${DISTRO}'")

# The following ones are distros that we have checked are built correctly

IF(${DISTRO} MATCHES "CentOS_6.*")
  TARGET_LINK_LIBRARIES(unitTest ${STATIC_LIBS} ${BOOST_MT} ${DYNAMIC_LIBS})
ELSEIF((${DISTRO} MATCHES "Debian_8.*") OR (${DISTRO} MATCHES "Debian_9.*"))
  # It seems that Debian 8.x/9.x doesn't like -mt libraries for unit tests... not sure about other Debian versions
  TARGET_LINK_LIBRARIES(unitTest ${STATIC_LIBS} ${BOOST} ${DYNAMIC_LIBS})
ELSEIF(${DISTRO} MATCHES "Debian_.*")
  TARGET_LINK_LIBRARIES(unitTest ${STATIC_LIBS} ${BOOST_MT} ${DYNAMIC_LIBS})

# The following ones are distros that haven't been checked since the refactoring in
# PR #2062. They should be "promoted" as soon as we have positive feedback about
# them or fixed (maybe based in the version of this file previous to
# the aforementioned PR)

ELSEIF ((${DISTRO} STREQUAL "Ubuntu_13.04") OR (${DISTRO} STREQUAL "Ubuntu_12.04.1_LTS") OR (${DISTRO} STREQUAL "Ubuntu_12.04.2_LTS"))
  # Pending to check after the refactor
  set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-conversion-null")  
  TARGET_LINK_LIBRARIES(unitTest ${STATIC_LIBS} ${BOOST_MT} ${DYNAMIC_LIBS})
ELSEIF(${DISTRO} STREQUAL "Ubuntu_13.10")  
  TARGET_LINK_LIBRARIES(unitTest ${STATIC_LIBS} ${BOOST} ${DYNAMIC_LIBS})
ELSEIF(${DISTRO} STREQUAL "Ubuntu_14.04.1_LTS")
  TARGET_LINK_LIBRARIES(unitTest ${STATIC_LIBS} ${BOOST} ${DYNAMIC_LIBS})
ELSEIF((${DISTRO} STREQUAL "Ubuntu_14.04.2_LTS") OR (${DISTRO} STREQUAL "Ubuntu_14.04.3_LTS") OR (${DISTRO} STREQUAL "Ubuntu_14.04.4_LTS"))
  TARGET_LINK_LIBRARIES(unitTest ${STATIC_LIBS} ${BOOST} ${DYNAMIC_LIBS})
ELSEIF(${DISTRO} STREQUAL "Ubuntu_14.10")  
  TARGET_LINK_LIBRARIES(unitTest ${STATIC_LIBS} ${BOOST} ${DYNAMIC_LIBS})
ELSEIF(${DISTRO} STREQUAL "openSUSE_12.3")  
  TARGET_LINK_LIBRARIES(unitTest ${STATIC_LIBS} ${BOOST_MT} ${DYNAMIC_LIBS})

# Finally this is a "best effort" option to try to build on distros that don't have a
# specify switch

ELSE()
  # Pending to check after the refactor
  TARGET_LINK_LIBRARIES(unitTest ${STATIC_LIBS} ${BOOST} ${DYNAMIC_LIBS})
ENDIF()
