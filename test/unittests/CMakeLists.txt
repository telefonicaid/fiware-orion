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
    serviceRoutines/deleteIndividualContextEntity_test.cpp
    serviceRoutines/deleteIndividualContextEntityAttribute_test.cpp
    serviceRoutines/getContextEntitiesByEntityId_test.cpp
    serviceRoutines/getEntityByIdAttributeByName_test.cpp
    serviceRoutines/getContextEntityTypes_test.cpp
    serviceRoutines/postContextEntityTypes_test.cpp
    serviceRoutines/getContextEntityTypeAttributeContainer_test.cpp
    serviceRoutines/postContextEntityTypeAttributeContainer_test.cpp
    serviceRoutines/getContextEntityTypeAttribute_test.cpp
    serviceRoutines/postContextEntityTypeAttribute_test.cpp
    serviceRoutines/getIndividualContextEntity_test.cpp
    serviceRoutines/getContextEntityAttributes_test.cpp
    serviceRoutines/getIndividualContextEntityAttribute_test.cpp
    serviceRoutines/postContextEntitiesByEntityId_test.cpp
    serviceRoutines/postContextEntityAttributes_test.cpp
    serviceRoutines/postDiscoverContextAvailability_test.cpp
    serviceRoutines/postEntityByIdAttributeByName_test.cpp
    serviceRoutines/postIndividualContextEntity_test.cpp
    serviceRoutines/postIndividualContextEntityAttribute_test.cpp
    serviceRoutines/logTraceTreat_test.cpp
    serviceRoutines/postRegisterContext_test.cpp
    serviceRoutines/postSubscribeContext_test.cpp
    serviceRoutines/postSubscribeContextAvailability_test.cpp
    serviceRoutines/postUnsubscribeContext_test.cpp
    serviceRoutines/postUnsubscribeContextAvailability_test.cpp
    serviceRoutines/postUpdateContext_test.cpp
    serviceRoutines/postUpdateContextAvailabilitySubscription_test.cpp
    serviceRoutines/postUpdateContextSubscription_test.cpp
    serviceRoutines/putIndividualContextEntity_test.cpp
    serviceRoutines/putIndividualContextEntityAttribute_test.cpp
    serviceRoutines/postQueryContext_test.cpp
    serviceRoutines/getNgsi10ContextEntityTypes_test.cpp
    serviceRoutines/getNgsi10ContextEntityTypesAttribute_test.cpp
    serviceRoutines/putSubscriptionConvOp_test.cpp
    serviceRoutines/badNgsi10Request_test.cpp
    serviceRoutines/badNgsi9Request_test.cpp
    serviceRoutines/badRequest_test.cpp
    serviceRoutines/versionTreat_test.cpp
    serviceRoutines/exitTreat_test.cpp
    serviceRoutines/leakTreat_test.cpp
    serviceRoutines/statisticsTreat_test.cpp
    serviceRoutines/putAvailabilitySubscriptionConvOp_test.cpp
    serviceRoutines/deleteAttributeValueInstance_test.cpp
    serviceRoutines/putAttributeValueInstance_test.cpp
    serviceRoutines/getAttributeValueInstance_test.cpp

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
    common/commonStatistics_test.cpp
    common/commonWsStrip_test.cpp

    ngsi9/RegisterContextRequest_test.cpp
    ngsi9/RegisterContextResponse_test.cpp
    ngsi9/DiscoverContextAvailabilityRequest_test.cpp
    ngsi9/DiscoverContextAvailabilityResponse_test.cpp
    ngsi9/SubscribeAvailabilityRequest_test.cpp
    ngsi9/SubscribeContextAvailabilityResponse_test.cpp
    ngsi9/NotifyContextAvailabilityRequest_test.cpp
    ngsi9/NotifyContextAvailabilityResponse_test.cpp
    ngsi9/UnsubscribeContextAvailabilityRequest_test.cpp
    ngsi9/UnsubscribeContextAvailabilityResponse_test.cpp
    ngsi9/UpdateContextAvailabilitySubscriptionRequest_test.cpp
    ngsi9/UpdateContextAvailabilitySubscriptionResponse_test.cpp

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
    ngsi/ContextElement_test.cpp
    ngsi/NotifyCondition_test.cpp
    ngsi/AttributeDomainName_test.cpp
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
    ngsi/ContextElementVector_test.cpp
    ngsi/ContextRegistration_test.cpp
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
    ngsi/UpdateActionType_test.cpp

    orionTypes/EntityType_test.cpp
    orionTypes/EntityTypeResponse_test.cpp
    orionTypes/EntityTypeVector_test.cpp
    orionTypes/EntityTypeVectorResponse_test.cpp
    orionTypes/QueryContextRequestVector_test.cpp
    orionTypes/QueryContextResponseVector_test.cpp
    orionTypes/UpdateContextRequestVector_test.cpp

    apiTypesV2/Entities_test.cpp
    apiTypesV2/Entity_test.cpp  
    apiTypesV2/EntityVector_test.cpp

    mongoBackend/mongoDiscoverContextAvailability_test.cpp
    mongoBackend/mongoQueryContext_test.cpp
    mongoBackend/mongoQueryContext_filters_test.cpp
    mongoBackend/mongoQueryContextCompoundValues_test.cpp
    mongoBackend/mongoQueryContextGeo_test.cpp
    mongoBackend/mongoRegisterContext_test.cpp
    mongoBackend/mongoRegisterContext_update_test.cpp
    mongoBackend/mongoSubscribeContextAvailability_test.cpp
    mongoBackend/mongoSubscribeContext_test.cpp
    mongoBackend/mongoUnsubscribeContextAvailability_test.cpp
    mongoBackend/mongoUnsubscribeContext_test.cpp
    mongoBackend/mongoUpdateContextAvailabilitySubscription_test.cpp
    mongoBackend/mongoUpdateContextSubscription_test.cpp
    mongoBackend/mongoUpdateContext_test.cpp
    mongoBackend/mongoUpdateContextGeo_test.cpp
    mongoBackend/mongoUpdateContext_withOnchangeSubscriptions_test.cpp
    mongoBackend/mongoUpdateContextCompoundValues_test.cpp
    mongoBackend/mongoNotifyContext_test.cpp
    mongoBackend/mongoNotifyContextAvailability_test.cpp
    mongoBackend/mongoContextProvidersQuery_test.cpp
    mongoBackend/mongoContextProvidersUpdate_test.cpp
    mongoBackend/mongoQueryTypes_test.cpp
    mongoBackend/mongoQueryContextFilterExistEntity_test.cpp
    mongoBackend/mongoGetSubscriptions_test.cpp

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
    rest
    jsonParse
    jsonParseV2
    apiTypesV2
    convenience
    serviceRoutines
    ngsiNotify
    rest
    parse
    ngsi9
    ngsi10
    ngsi
    cache
    mongoBackend
    orionTypes
    common
    alarmMgr
    logSummary
    lm
    pa
    pthread
    curl
    microhttpd.a
    gnutls        
)

# Include directories
# ------------------------------------------------------------
include_directories("${PROJECT_SOURCE_DIR}")
include_directories("${PROJECT_SOURCE_DIR}/src/lib")
include_directories("${PROJECT_SOURCE_DIR}/src/app")
include_directories("${PROJECT_SOURCE_DIR}/test/unittests")
include_directories("${RAPIDXML_INCLUDE}")

# Lib directories
# ------------------------------------------------------------
link_directories("/usr/local/lib/")
link_directories("/usr/lib64/")
link_directories("/usr/lib/x86_64-linux-gnu")

# FIXME P4: Issue #476: remove or keep?
link_directories("/opt/lib/")


# flags
# ------------------------------------------------------------
add_definitions(-DUNIT_TEST)



# Executable declaration
# ------------------------------------------------------------

ADD_EXECUTABLE(unitTest ${SOURCES} ${HEADERS})

IF ((${DISTRO} STREQUAL "Ubuntu_13.04") OR (${DISTRO} STREQUAL "Ubuntu_12.04.1_LTS") OR (${DISTRO} STREQUAL "Ubuntu_12.04.2_LTS"))
  set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-conversion-null")
  MESSAGE("unitTest: Ubuntu DISTRO: '${DISTRO}'")
  TARGET_LINK_LIBRARIES(unitTest ${STATIC_LIBS} -lmongoclient -lboost_thread-mt -lboost_filesystem-mt -lboost_system-mt -lboost_regex-mt -lpthread -lssl -lcrypto -lgnutls -lgcrypt -luuid -lwebsockets)
ELSEIF(${DISTRO} STREQUAL "Ubuntu_13.10")
  MESSAGE("unitTest: Ubuntu DISTRO: '${DISTRO}'")
  TARGET_LINK_LIBRARIES(unitTest ${STATIC_LIBS} -lmongoclient -lboost_thread -lboost_filesystem -lboost_system -lpthread -lssl -lcrypto -lgnutls -lgcrypt -luuid -lwebsockets)
ELSEIF(${DISTRO} STREQUAL "Ubuntu_14.04.1_LTS")
  # FIXME P4: Issue #476: remove or keep?
  MESSAGE("unitTest: Ubuntu DISTRO: '${DISTRO}'")
  TARGET_LINK_LIBRARIES(unitTest ${STATIC_LIBS} -lmongoclient -lboost_thread -lboost_filesystem -lboost_system -lpthread -lssl -lcrypto -lgnutls -lgcrypt -luuid -lwebsockets)
ELSEIF((${DISTRO} STREQUAL "Ubuntu_14.04.2_LTS") OR (${DISTRO} STREQUAL "Ubuntu_14.04.3_LTS"))
  # FIXME P4: Issue #476: remove or keep?
  MESSAGE("unitTest: Ubuntu DISTRO: '${DISTRO}'")
  TARGET_LINK_LIBRARIES(unitTest ${STATIC_LIBS} -lmongoclient -lboost_thread -lboost_filesystem -lboost_system -lboost_regex -lpthread -lssl -lcrypto -lgnutls -lgcrypt -luuid -lwebsockets)
ELSEIF(${DISTRO} STREQUAL "Ubuntu_14.10")
  # ADDED FOR UBUNTU 14.10
  MESSAGE("contextBroker: Ubuntu DISTRO: '${DISTRO}'")
  TARGET_LINK_LIBRARIES(unitTest ${STATIC_LIBS} -lmongoclient -lboost_thread -lboost_filesystem -lboost_system -lpthread -lssl -lcrypto -lgnutls -lgcrypt -luuid -lwebsockets)
ELSEIF(${DISTRO} STREQUAL "openSUSE_12.3")
  MESSAGE("unitTest: openSUSE DISTRO: '${DISTRO}'")
  TARGET_LINK_LIBRARIES(unitTest ${STATIC_LIBS} -lmongoclient -lboost_thread-mt -lboost_filesystem-mt -lboost_system-mt -lpthread -lssl -lcrypto -luuid -lwebsockets)
ELSEIF(${DISTRO} MATCHES "CentOS_6.*")
  MESSAGE("unitTest: CentOS DISTRO: '${DISTRO}'")
  TARGET_LINK_LIBRARIES(unitTest ${STATIC_LIBS} -lmongoclient -lboost_thread-mt -lboost_filesystem-mt -lboost_system-mt -lboost_regex-mt -luuid -lwebsockets)
ELSEIF(${DISTRO} MATCHES "Debian_8.2")
  # It seems that Debian 8.2 doesn't like -mt libraries for unit tests... not sure about other Debian versions
  MESSAGE("unitTest: Debian DISTRO: '${DISTRO}'")
  TARGET_LINK_LIBRARIES(unitTest ${STATIC_LIBS} -lmongoclient -lboost_thread -lboost_filesystem -lboost_system -lboost_regex -lgcrypt -luuid -lwebsockets)
ELSE()
ELSEIF(${DISTRO} MATCHES "Debian_.*")
  MESSAGE("unitTest: Debian DISTRO: '${DISTRO}'")
  TARGET_LINK_LIBRARIES(unitTest ${STATIC_LIBS} -lmongoclient -lboost_thread-mt -lboost_filesystem-mt -lboost_system-mt -lboost_regex-mt -lgcrypt -luuid -lwebsockets)
ELSE()
  MESSAGE("unitTest: Other DISTRO: '${DISTRO}'")
  TARGET_LINK_LIBRARIES(unitTest ${STATIC_LIBS} -lmongoclient -lboost_thread -lboost_filesystem -lboost_system -lpthread -luuid -lwebsockets)
ENDIF()
