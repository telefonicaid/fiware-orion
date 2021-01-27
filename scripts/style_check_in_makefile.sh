#!/bin/bash
# -*- coding: latin-1 -*-
# Copyright 2016 Telefonica Investigacion y Desarrollo, S.A.U
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
# Author: Ken Zangelin

# NOTE: this script is designed to be launched from makefile targets. Thus,
# the call to style_check.sh may break if you attempt to use it from a
# different place. 


# -----------------------------------------------------------------------------
#
# style_check
#
function style_check
{
  scripts/style_check.sh -d "$1"
  if [ "$?" != 0 ]
  then
    echo Lint Errors:
    cat LINT_ERRORS
    echo "================================================================="
    cat LINT | grep -v "Done processing"
    echo 
    exit 1
  fi
}

style_check src/lib/serviceRoutinesV2
style_check src/lib/logSummary
style_check src/lib/jsonParseV2
style_check src/lib/apiTypesV2
style_check src/lib/mongoBackend
style_check src/lib/logMsg
style_check src/lib/parseArgs
style_check src/lib/cache
style_check src/lib/alarmMgr
style_check src/lib/metricsMgr
style_check test/unittests
style_check test/unittests/orionTypes
style_check test/unittests/jsonParse
style_check test/unittests/apiTypesV2
style_check test/unittests/cache
style_check test/unittests/mongoBackend
style_check test/unittests/rest
style_check test/unittests/serviceRoutines

# FIXME: Just keep adding directories here until all of them are included:

# style_check src/app/contextBroker (1 file)
# style_check src/lib/ngsiNotify (1 file)
# style_check src/lib/parse (2 files)
# style_check src/lib/rest (3 files)
# style_check src/lib/common (6 files)
# style_check src/lib/orionTypes (9 files)
# style_check src/lib/convenience (17 files)
# style_check src/lib/jsonParse (19 files)
# style_check src/lib/ngsi10 (24 files)
# style_check src/lib/ngsi9 (24 files)
# style_check src/lib/serviceRoutines (46 files)
# style_check src/lib/ngsi (74 files)

# style_check test/unittests/common (1 file)
# style_check test/unittests/parse (2 files)
# style_check test/unittests/convenience (8 files)
# style_check test/unittests/ngsi10 (9 files)
# style_check test/unittests/ngsi9 (10 files)
# style_check test/unittests/ngsi (32 files)
#
# style_check test/unittests (1 file, but already done)
#



# -----------------------------------------------------------------------------
#
# Blockers
#

#
# o app/contextBroker.cpp                                                    (feature/mqtt_notifications_poc: 1 include, 1 var, 2 new functions, 1 func-call in main())
# o app/contextBroker.cpp                                                    (feature/2745_lib_versions: 1 include, 1 new function, 1 line added to main())
# o app/contextBroker.cpp                                                    (haderding/remove_ngsiv1_indent: 1 new parse-arg)
#
# o common/string.cpp                                                        (feature/mqtt_notifications_poc: addition in parseUrl())
# o common/string.cpp                                                        (haderding/remove_ngsiv1_indent: 1 new function)
# o common/string.h                                                          (haderding/remove_ngsiv1_indent: 1 include, 1 external declaration)
# o common/globals.h                                                         (feature/mqtt_notifications_poc: 1 external declaration)
# o common/globals.h                                                         (haderding/remove_ngsiv1_indent: 1 new external declaration)
# o common/tag.cpp                                                           (haderding/remove_ngsiv1_indent: 'indent' removed from 6 signatures, slight change in 6 functions)
# o common/tag.h                                                             (haderding/remove_ngsiv1_indent: 'indent' removed from 6 signatures)
# o common/macroSubstitute.cpp                                               (haderding/remove_ngsiv1_indent: 2 lines: a param added to toJson())
#
# o rest/httpRequestSend.cpp                                                 (feature/mqtt_notifications_poc: 1 include, 1 new function, addition in httpRequestSendWithCurl())
# o rest/restReply.cpp                                                       (haderding/remove_ngsiv1_indent: 3 includes, 55 lines in restReply(), 13 lines in restErrorReplyGet())
# o rest/orionLogReply.cpp                                                   (haderding/remove_ngsiv1_indent: 1 line)
# o rest/OrionError.cpp                                                      (haderding/remove_ngsiv1_indent: 8 lines)
#
# o convenience/AppendContextElementRequest.cpp                              (haderding/remove_ngsiv1_indent: 15 lines)
# o convenience/AppendContextElementRequest.h                                (haderding/remove_ngsiv1_indent: 3 lines)
# o convenience/AppendContextElementResponse.cpp                             (haderding/remove_ngsiv1_indent: 12 lines)
# o convenience/AppendContextElementResponse.h                               (haderding/remove_ngsiv1_indent: 2 lines)
# o convenience/ContextAttributeResponse.cpp                                 (haderding/remove_ngsiv1_indent: 10 lines)
# o convenience/ContextAttributeResponse.h                                   (haderding/remove_ngsiv1_indent: 6 lines)
# o convenience/ContextAttributeResponseVector.cpp                           (haderding/remove_ngsiv1_indent: 8 lines)
# o convenience/ContextAttributeResponseVector.h                             (haderding/remove_ngsiv1_indent: 6 lines)
# o convenience/RegisterProviderRequest.cpp                                  (haderding/remove_ngsiv1_indent: 14 lines)
# o convenience/RegisterProviderRequest.h                                    (haderding/remove_ngsiv1_indent: 3 lines)
# o convenience/UpdateContextAttributeRequest.cpp                            (haderding/remove_ngsiv1_indent: 16 lines)
# o convenience/UpdateContextAttributeRequest.h                              (haderding/remove_ngsiv1_indent: 3 lines)
# o convenience/UpdateContextElementRequest.cpp                              (haderding/remove_ngsiv1_indent: 8 lines)
# o convenience/UpdateContextElementRequest.h                                (haderding/remove_ngsiv1_indent: 3 lines)
# o convenience/UpdateContextElementResponse.cpp                             (haderding/remove_ngsiv1_indent: 9 lines)
# o convenience/UpdateContextElementResponse.h                               (haderding/remove_ngsiv1_indent: 4 lines)
#
# o jsonParse/jsonAppendContextElementRequest.cpp                            (haderding/remove_ngsiv1_indent: 1 call to check())
# o jsonParse/jsonDiscoverContextAvailabilityRequest.cpp                     (haderding/remove_ngsiv1_indent: 1 call to check())
# o jsonParse/jsonNotifyContextRequest.cpp                                   (haderding/remove_ngsiv1_indent: 1 call to check())
# o jsonParse/jsonQueryContextRequest.cpp                                    (haderding/remove_ngsiv1_indent: 1 call to check())
# o jsonParse/jsonQueryContextResponse.cpp                                   (haderding/remove_ngsiv1_indent: 1 call to check())
# o jsonParse/jsonRegisterContextRequest.cpp                                 (haderding/remove_ngsiv1_indent: 1 call to check())
# o jsonParse/jsonRegisterProviderRequest.cpp                                (haderding/remove_ngsiv1_indent: 1 call to check())
# o jsonParse/jsonSubscribeContextRequest.cpp                                (haderding/remove_ngsiv1_indent: 2 calls to check()) 
# o jsonParse/jsonUnsubscribeContextRequest.cpp                              (haderding/remove_ngsiv1_indent: 1 call to check())
# o jsonParse/jsonUpdateContextAttributeRequest.cpp                          (haderding/remove_ngsiv1_indent: 1 call to check())
# o jsonParse/jsonUpdateContextElementRequest.cpp                            (haderding/remove_ngsiv1_indent: 1 call to check())
# o jsonParse/jsonUpdateContextRequest.cpp                                   (haderding/remove_ngsiv1_indent: 1 call to check())
# o jsonParse/jsonUpdateContextResponse.cpp                                  (haderding/remove_ngsiv1_indent: 1 call to check())
# o jsonParse/jsonUpdateContextSubscriptionRequest.cpp                       (haderding/remove_ngsiv1_indent: 2 calls to check())
#
# o ngsi/AttributeDomainName.cpp                                             (haderding/remove_ngsiv1_indent: 9 lines)
# o ngsi/AttributeDomainName.h                                               (haderding/remove_ngsiv1_indent: 5 lines)
# o ngsi/AttributeExpression.cpp                                             (haderding/remove_ngsiv1_indent: 9 lines)
# o ngsi/AttributeExpression.h                                               (haderding/remove_ngsiv1_indent: 6 lines)
# o ngsi/StringList.cpp                                                      (haderding/remove_ngsiv1_indent: 13 lines)
# o ngsi/StringList.h                                                        (haderding/remove_ngsiv1_indent: 5 lines)
# o ngsi/ConditionValueList.cpp                                              (haderding/remove_ngsiv1_indent: 13 lines)
# o ngsi/ConditionValueList.h                                                (haderding/remove_ngsiv1_indent: 5 lines)
# o ngsi/ContextAttribute.cpp                                                (haderding/remove_ngsiv1_indent: 47 lines)
# o ngsi/ContextAttribute.h                                                  (haderding/remove_ngsiv1_indent: 8 lines)
# o ngsi/ContextAttributeVector.cpp                                          (haderding/remove_ngsiv1_indent: 15 lines)
# o ngsi/ContextAttributeVector.h                                            (haderding/remove_ngsiv1_indent: 7 lines)
# o ngsi/ContextElement.cpp                                                  (haderding/remove_ngsiv1_indent: 23 lines)
# o ngsi/ContextElement.h                                                    (haderding/remove_ngsiv1_indent: 6 lines)
# o ngsi/ContextElementResponse.cpp                                          (haderding/remove_ngsiv1_indent: 8 lines)
# o ngsi/ContextElementResponse.h                                            (haderding/remove_ngsiv1_indent: 7 lines)
# o ngsi/ContextElementResponseVector.cpp                                    (haderding/remove_ngsiv1_indent: 11 lines)
# o ngsi/ContextElementResponseVector.h                                      (haderding/remove_ngsiv1_indent: 7 lines)
# o ngsi/ContextElementVector.cpp                                            (haderding/remove_ngsiv1_indent: 13 lines)
# o ngsi/ContextElementVector.h                                              (haderding/remove_ngsiv1_indent: 8 lines)
# o ngsi/ContextRegistration.cpp                                             (haderding/remove_ngsiv1_indent: 10 lines)
# o ngsi/ContextRegistration.h                                               (haderding/remove_ngsiv1_indent: 2 lines)
# o ngsi/ContextRegistrationAttribute.cpp                                    (haderding/remove_ngsiv1_indent: 7 lines)
# o ngsi/ContextRegistrationAttribute.h                                      (haderding/remove_ngsiv1_indent: 1 line)
# o ngsi/ContextRegistrationAttributeVector.cpp                              (haderding/remove_ngsiv1_indent: 4 lines)
# o ngsi/ContextRegistrationAttributeVector.h                                (haderding/remove_ngsiv1_indent: 1 line)
# o ngsi/ContextRegistrationResponse.cpp                                     (haderding/remove_ngsiv1_indent: 7 lines)
# o ngsi/ContextRegistrationResponse.h                                       (haderding/remove_ngsiv1_indent: 2 lines)
# o ngsi/ContextRegistrationResponseVector.cpp                               (haderding/remove_ngsiv1_indent: 6 lines)
# o ngsi/ContextRegistrationResponseVector.h                                 (haderding/remove_ngsiv1_indent: 2 lines)
# o ngsi/ContextRegistrationVector.cpp                                       (haderding/remove_ngsiv1_indent: 6 lines)
# o ngsi/ContextRegistrationVector.h                                         (haderding/remove_ngsiv1_indent: 2 lines)
# o ngsi/Duration.cpp                                                        (haderding/remove_ngsiv1_indent: 9 lines)
# o ngsi/Duration.h                                                          (haderding/remove_ngsiv1_indent: 5 lines)
# o ngsi/EntityId.cpp                                                        (haderding/remove_ngsiv1_indent: 24 lines)
# o ngsi/EntityId.h                                                          (haderding/remove_ngsiv1_indent: 5 lines)
# o ngsi/EntityIdVector.cpp                                                  (haderding/remove_ngsiv1_indent: 10 lines)
# o ngsi/EntityIdVector.h                                                    (haderding/remove_ngsiv1_indent: 2 lines)
# o ngsi/Metadata.cpp                                                        (haderding/remove_ngsiv1_indent: 37 lines)
# o ngsi/Metadata.h                                                          (haderding/remove_ngsiv1_indent: 1 line)
# o ngsi/MetadataVector.cpp                                                  (haderding/remove_ngsiv1_indent: 4 lines)
# o ngsi/MetadataVector.h                                                    (haderding/remove_ngsiv1_indent: 1 line)
# o ngsi/NotifyCondition.cpp                                                 (haderding/remove_ngsiv1_indent: 9 lines)
# o ngsi/NotifyCondition.h                                                   (haderding/remove_ngsiv1_indent: 2 lines)
# o ngsi/NotifyConditionVector.cpp                                           (haderding/remove_ngsiv1_indent: 6 lines)
# o ngsi/NotifyConditionVector.h                                             (haderding/remove_ngsiv1_indent: 2 lines)
# o ngsi/Originator.cpp                                                      (haderding/remove_ngsiv1_indent: 9 lines)
# o ngsi/Originator.h                                                        (haderding/remove_ngsiv1_indent: 5 lines)
# o ngsi/ProvidingApplication.cpp                                            (haderding/remove_ngsiv1_indent: 9 lines)
# o ngsi/ProvidingApplication.h                                              (haderding/remove_ngsiv1_indent: 5 lines)
# o ngsi/Reference.cpp                                                       (haderding/remove_ngsiv1_indent: 9 lines)
# o ngsi/Reference.h                                                         (haderding/remove_ngsiv1_indent: 5 lines)
# o ngsi/RegistrationId.cpp                                                  (haderding/remove_ngsiv1_indent: 9 lines)
# o ngsi/RegistrationId.h                                                    (haderding/remove_ngsiv1_indent: 5 lines)
# o ngsi/Restriction.cpp                                                     (haderding/remove_ngsiv1_indent: 14 lines)
# o ngsi/Restriction.h                                                       (haderding/remove_ngsiv1_indent: 5 lines)
# o ngsi/RestrictionString.cpp                                               (haderding/remove_ngsiv1_indent: 9 lines)
# o ngsi/RestrictionString.h                                                 (haderding/remove_ngsiv1_indent: 5 lines)
# o ngsi/Scope.cpp                                                           (haderding/remove_ngsiv1_indent: 12 lines)
# o ngsi/Scope.h                                                             (haderding/remove_ngsiv1_indent: 5 lines)
# o ngsi/ScopeVector.cpp                                                     (haderding/remove_ngsiv1_indent: 12 lines)
# o ngsi/ScopeVector.h                                                       (haderding/remove_ngsiv1_indent: 5 lines)
# o ngsi/StatusCode.cpp                                                      (haderding/remove_ngsiv1_indent: 13 lines)
# o ngsi/StatusCode.h                                                        (haderding/remove_ngsiv1_indent: 5 lines)
# o ngsi/SubscribeError.cpp                                                  (haderding/remove_ngsiv1_indent: 12 lines)
# o ngsi/SubscribeError.h                                                    (haderding/remove_ngsiv1_indent: 5 lines)
# o ngsi/SubscribeResponse.cpp                                               (haderding/remove_ngsiv1_indent: 6 lines)
# o ngsi/SubscribeResponse.h                                                 (haderding/remove_ngsiv1_indent: 1 line)
# o ngsi/SubscriptionId.cpp                                                  (haderding/remove_ngsiv1_indent: 9 lines)
# o ngsi/SubscriptionId.h                                                    (haderding/remove_ngsiv1_indent: 5 lines)
# o ngsi/Throttling.cpp                                                      (haderding/remove_ngsiv1_indent: 9 lines)
# o ngsi/Throttling.h                                                        (haderding/remove_ngsiv1_indent: 5 lines)
# o ngsi/UpdateActionType.cpp                                                (haderding/remove_ngsiv1_indent: 2 lines)
# o ngsi/UpdateActionType.h                                                  (haderding/remove_ngsiv1_indent: 1 line)
#
# o ngsi10/NotifyContextRequest.cpp                                          (haderding/remove_ngsiv1_indent: 11 lines)
# o ngsi10/NotifyContextRequest.h                                            (haderding/remove_ngsiv1_indent: 2 lines)
# o ngsi10/NotifyContextResponse.cpp                                         (haderding/remove_ngsiv1_indent: 4 lines)
# o ngsi10/NotifyContextResponse.h                                           (haderding/remove_ngsiv1_indent: 1 line)
# o ngsi10/QueryContextRequest.cpp                                           (haderding/remove_ngsiv1_indent: 11 lines)
# o ngsi10/QueryContextRequest.h                                             (haderding/remove_ngsiv1_indent: 2 lines)
# o ngsi10/QueryContextResponse.cpp                                          (haderding/remove_ngsiv1_indent: 9 lines)
# o ngsi10/QueryContextResponse.h                                            (haderding/remove_ngsiv1_indent: 2 lines)
# o ngsi10/SubscribeContextRequest.cpp                                       (haderding/remove_ngsiv1_indent: 9 lines)
# o ngsi10/SubscribeContextRequest.h                                         (haderding/remove_ngsiv1_indent: 1 line)
# o ngsi10/SubscribeContextResponse.cpp                                      (haderding/remove_ngsiv1_indent: 5 lines)
# o ngsi10/SubscribeContextResponse.h                                        (haderding/remove_ngsiv1_indent: 1 line)
# o ngsi10/UnsubscribeContextRequest.cpp                                     (haderding/remove_ngsiv1_indent: 7 lines)
# o ngsi10/UnsubscribeContextRequest.h                                       (haderding/remove_ngsiv1_indent: 2 lines)
# o ngsi10/UnsubscribeContextResponse.cpp                                    (haderding/remove_ngsiv1_indent: 5 lines)
# o ngsi10/UnsubscribeContextResponse.h                                      (haderding/remove_ngsiv1_indent: 1 line)
# o ngsi10/UpdateContextRequest.cpp                                          (haderding/remove_ngsiv1_indent: 9 lines)
# o ngsi10/UpdateContextRequest.h                                            (haderding/remove_ngsiv1_indent: 2 lines)
# o ngsi10/UpdateContextResponse.cpp                                         (haderding/remove_ngsiv1_indent: 9 lines)
# o ngsi10/UpdateContextResponse.h                                           (haderding/remove_ngsiv1_indent: 2 lines)
# o ngsi10/UpdateContextSubscriptionRequest.cpp                              (haderding/remove_ngsiv1_indent: 7 lines)
# o ngsi10/UpdateContextSubscriptionRequest.h                                (haderding/remove_ngsiv1_indent: 1 line)
# o ngsi10/UpdateContextSubscriptionResponse.cpp                             (haderding/remove_ngsiv1_indent: 5 lines)
# o ngsi10/UpdateContextSubscriptionResponse.h                               (haderding/remove_ngsiv1_indent: 1 line)
#
# o ngsi9/DiscoverContextAvailabilityRequest.cpp                             (haderding/remove_ngsiv1_indent: 5 lines)
# o ngsi9/DiscoverContextAvailabilityRequest.h                               (haderding/remove_ngsiv1_indent: 2 lines)
# o ngsi9/DiscoverContextAvailabilityResponse.cpp                            (haderding/remove_ngsiv1_indent: 6 lines)
# o ngsi9/DiscoverContextAvailabilityResponse.h                              (haderding/remove_ngsiv1_indent: 2 lines)
# o ngsi9/RegisterContextRequest.cpp                                         (haderding/remove_ngsiv1_indent: 11 lines)
# o ngsi9/RegisterContextRequest.h                                           (haderding/remove_ngsiv1_indent: 2 lines)
# o ngsi9/RegisterContextResponse.cpp                                        (haderding/remove_ngsiv1_indent: 10 lines)
# o ngsi9/RegisterContextResponse.h                                          (haderding/remove_ngsiv1_indent: 2 lines)
#
# o ngsiNotify/Notifier.cpp                                                  (haderding/remove_ngsiv1_indent: 3 includes, 
#                                                                                                             35 lines in buildSenderParams())
#
# o orionTypes/EntityType.cpp                                                (haderding/remove_ngsiv1_indent: 13 lines)
# o orionTypes/EntityType.h                                                  (haderding/remove_ngsiv1_indent: 6 lines)
# o orionTypes/EntityTypeResponse.cpp                                        (haderding/remove_ngsiv1_indent: 7 lines)
# o orionTypes/EntityTypeResponse.h                                          (haderding/remove_ngsiv1_indent: 2 lines)
# o orionTypes/EntityTypeVector.cpp                                          (haderding/remove_ngsiv1_indent: 9 lines)
# o orionTypes/EntityTypeVector.h                                            (haderding/remove_ngsiv1_indent: 6 lines)
# o orionTypes/EntityTypeVectorResponse.cpp                                  (haderding/remove_ngsiv1_indent: 10 lines)
# o orionTypes/EntityTypeVectorResponse.h                                    (haderding/remove_ngsiv1_indent: 5 lines)
# o orionTypes/QueryContextResponseVector.cpp                                (haderding/remove_ngsiv1_indent: 1 line)
#
# o parse/CompoundValueNode.cpp                                              (haderding/remove_ngsiv1_indent: 41 lines)
# o parse/CompoundValueNode.h                                                (haderding/remove_ngsiv1_indent: 2 lines)
#
# o rest/OrionError.cpp                                                      (haderding/remove_ngsiv1_indent: 8 lines)
# o rest/orionLogReply.cpp                                                   (haderding/remove_ngsiv1_indent: 1 line)
# o rest/restReply.cpp                                                       (haderding/remove_ngsiv1_indent: 3 includes,
#                                                                                                               alt exec-part in restReply(),
#                                                                                                               'indent' removed from 13 calls to render())
#
# o serviceRoutines/deleteAllEntitiesWithTypeAndId.cpp                       (haderding/remove_ngsiv1_indent: 3 TIMED_RENDER)
# o serviceRoutines/deleteAttributeValueInstance.cpp                         (haderding/remove_ngsiv1_indent: 1 TIMED_RENDER)
# o serviceRoutines/deleteAttributeValueInstanceWithTypeAndId.cpp            (haderding/remove_ngsiv1_indent: 2 TIMED_RENDER)
# o serviceRoutines/deleteIndividualContextEntity.cpp                        (haderding/remove_ngsiv1_indent: 1 TIMED_RENDER)
# o serviceRoutines/deleteIndividualContextEntityAttribute.cpp               (haderding/remove_ngsiv1_indent: 1 TIMED_RENDER)
# o serviceRoutines/deleteIndividualContextEntityAttributeWithTypeAndId.cpp  (haderding/remove_ngsiv1_indent: 3 TIMED_RENDER)
# o serviceRoutines/getAllEntitiesWithTypeAndId.cpp                          (haderding/remove_ngsiv1_indent: 1 TIMED_RENDER)
# o serviceRoutines/getAttributeValueInstance.cpp                            (haderding/remove_ngsiv1_indent: 1 TIMED_RENDER)
# o serviceRoutines/getAttributeValueInstanceWithTypeAndId.cpp               (haderding/remove_ngsiv1_indent: 1 TIMED_RENDER)
# o serviceRoutines/getAttributesForEntityType.cpp                           (haderding/remove_ngsiv1_indent: 1 TIMED_RENDER)
# o serviceRoutines/getContextEntitiesByEntityIdAndType.cpp                  (haderding/remove_ngsiv1_indent: 2 TIMED_RENDER)
# o serviceRoutines/getEntityByIdAttributeByNameWithTypeAndId.cpp            (haderding/remove_ngsiv1_indent: 2 TIMED_RENDER)
# o serviceRoutines/getEntityTypes.cpp                                       (haderding/remove_ngsiv1_indent: 1 TIMED_RENDER)
# o serviceRoutines/getIndividualContextEntity.cpp                           (haderding/remove_ngsiv1_indent: 1 TIMED_RENDER)
# o serviceRoutines/getIndividualContextEntityAttribute.cpp                  (haderding/remove_ngsiv1_indent: 1 TIMED_RENDER)
# o serviceRoutines/getIndividualContextEntityAttributeWithTypeAndId.cpp     (haderding/remove_ngsiv1_indent: 1 TIMED_RENDER)
# o serviceRoutines/getNgsi10ContextEntityTypes.cpp                          (haderding/remove_ngsiv1_indent: 3 TIMED_RENDER)
# o serviceRoutines/getNgsi10ContextEntityTypesAttribute.cpp                 (haderding/remove_ngsiv1_indent: 3 TIMED_RENDER)
# o serviceRoutines/postAllEntitiesWithTypeAndId.cpp                         (haderding/remove_ngsiv1_indent: 4 TIMED_RENDER)
# o serviceRoutines/postAttributeValueInstanceWithTypeAndId.cpp              (haderding/remove_ngsiv1_indent: 3 TIMED_RENDER)
# o serviceRoutines/postContextEntitiesByEntityIdAndType.cpp                 (haderding/remove_ngsiv1_indent: 2 TIMED_RENDER)
# o serviceRoutines/postDiscoverContextAvailability.cpp                      (haderding/remove_ngsiv1_indent: 1 TIMED_RENDER)
# o serviceRoutines/postEntityByIdAttributeByNameWithTypeAndId.cpp           (haderding/remove_ngsiv1_indent: 2 TIMED_RENDER)
# o serviceRoutines/postIndividualContextEntity.cpp                          (haderding/remove_ngsiv1_indent: 5 TIMED_RENDER)
# o serviceRoutines/postIndividualContextEntityAttribute.cpp                 (haderding/remove_ngsiv1_indent: 1 TIMED_RENDER)
# o serviceRoutines/postIndividualContextEntityAttributeWithTypeAndId.cpp    (haderding/remove_ngsiv1_indent: 1 TIMED_RENDER)
# o serviceRoutines/postNotifyContext.cpp                                    (haderding/remove_ngsiv1_indent: 1 TIMED_RENDER)
# o serviceRoutines/postQueryContext.cpp                                     (haderding/remove_ngsiv1_indent: 3 includes, 2 TIMED_RENDER, 35 lines in queryForward())
# o serviceRoutines/postRegisterContext.cpp                                  (haderding/remove_ngsiv1_indent: 3 TIMED_RENDER)
# o serviceRoutines/postSubscribeContext.cpp                                 (haderding/remove_ngsiv1_indent: 2 TIMED_RENDER)
# o serviceRoutines/postUnsubscribeContext.cpp                               (haderding/remove_ngsiv1_indent: 1 TIMED_RENDER)
# o serviceRoutines/postUpdateContext.cpp                                    (haderding/remove_ngsiv1_indent: 3 includes, 5 TIMED_RENDER, 35 lines in updateForward)
# o serviceRoutines/postUpdateContextSubscription.cpp                        (haderding/remove_ngsiv1_indent: 1 TIMED_RENDER)
# o serviceRoutines/putAllEntitiesWithTypeAndId.cpp                          (haderding/remove_ngsiv1_indent: 3 TIMED_RENDER)
# o serviceRoutines/putAttributeValueInstance.cpp                            (haderding/remove_ngsiv1_indent: 2 TIMED_RENDER)
# o serviceRoutines/putAttributeValueInstanceWithTypeAndId.cpp               (haderding/remove_ngsiv1_indent: 3 TIMED_RENDER)
# o serviceRoutines/putIndividualContextEntity.cpp                           (haderding/remove_ngsiv1_indent: 1 TIMED_RENDER)
# o serviceRoutines/putIndividualContextEntityAttribute.cpp                  (haderding/remove_ngsiv1_indent: 1 TIMED_RENDER)
# o serviceRoutines/putIndividualContextEntityAttributeWithTypeAndId.cpp     (haderding/remove_ngsiv1_indent: 1 TIMED_RENDER)
# o serviceRoutines/versionTreat.cpp                                         (haderding/remove_ngsiv1_indent: 8 lines in versionTreat())
# o serviceRoutinesV2/getEntityAttributeValue.cpp                            (haderding/remove_ngsiv1_indent: 1 TIMED_RENDER)
# o serviceRoutinesV2/postSubscriptions.cpp                                  (haderding/remove_ngsiv1_indent: 1 TIMED_RENDER)
#
#
# o unittests/main_UnitTest.cpp                                              (haderding/remove_ngsiv1_indent: 1 variable)
#
# o unittests/common/commonTag_test.cpp                                      (haderding/remove_ngsiv1_indent: 24 lineas)
#
# o unittests/convenience/AppendContextElementRequest_test.cpp               (haderding/remove_ngsiv1_indent: 1 render(), 4 check())
# o unittests/convenience/AppendContextElementResponse_test.cpp              (haderding/remove_ngsiv1_indent: 2 render(), 3 check())
# o unittests/convenience/ContextAttributeResponseVector_test.cpp            (haderding/remove_ngsiv1_indent: 2 render(), 3 check())
# o unittests/convenience/ContextAttributeResponse_test.cpp                  (haderding/remove_ngsiv1_indent: 1 render(), 3 check())
# o unittests/convenience/RegisterProviderRequest_test.cpp                   (haderding/remove_ngsiv1_indent: 2 render(), 2 check())
# o unittests/convenience/UpdateContextAttributeRequest_test.cpp             (haderding/remove_ngsiv1_indent: 1 render(), 4 check())
# o unittests/convenience/UpdateContextElementRequest_test.cpp               (haderding/remove_ngsiv1_indent: 1 render(), 3 check())
# o unittests/convenience/UpdateContextElementResponse_test.cpp              (haderding/remove_ngsiv1_indent: 1 render(), 3 check())
#
# o unittests/ngsi/AttributeDomainName_test.cpp                              (haderding/remove_ngsiv1_indent: 2 render())
# o unittests/ngsi/AttributeExpression_test.cpp                              (haderding/remove_ngsiv1_indent: 2 render()
# o unittests/ngsi/AttributeList_test.cpp                                    (haderding/remove_ngsiv1_indent: 2 render(), 1 check())
# o unittests/ngsi/ConditionValueList_test.cpp                               (haderding/remove_ngsiv1_indent: 3 render(), 2 check())
# o unittests/ngsi/ContextElementResponseVector_test.cpp                     (haderding/remove_ngsiv1_indent: 2 check())
# o unittests/ngsi/ContextElementResponse_test.cpp                           (haderding/remove_ngsiv1_indent: 3 check())
# o unittests/ngsi/ContextElementVector_test.cpp                             (haderding/remove_ngsiv1_indent: 2 render()
# o unittests/ngsi/ContextElement_test.cpp                                   (haderding/remove_ngsiv1_indent: 8 check())
# o unittests/ngsi/ContextRegistrationAttributeVector_test.cpp               (haderding/remove_ngsiv1_indent: 4 render()
# o unittests/ngsi/ContextRegistrationAttribute_test.cpp                     (haderding/remove_ngsiv1_indent: 1 render()
# o unittests/ngsi/ContextRegistrationResponseVector_test.cpp                (haderding/remove_ngsiv1_indent: 1 render(), 3 check())
# o unittests/ngsi/ContextRegistrationResponse_test.cpp                      (haderding/remove_ngsiv1_indent: 2 render(), 1 check())
# o unittests/ngsi/ContextRegistrationVector_test.cpp                        (haderding/remove_ngsiv1_indent: 1 render()
# o unittests/ngsi/Duration_test.cpp                                         (haderding/remove_ngsiv1_indent: 3 check())
# o unittests/ngsi/EntityId_test.cpp                                         (haderding/remove_ngsiv1_indent: 1 render()
# o unittests/ngsi/ErrorCode_test.cpp                                        (haderding/remove_ngsiv1_indent: 1 render()
# o unittests/ngsi/MetadataVector_test.cpp                                   (haderding/remove_ngsiv1_indent: 2 render()
# o unittests/ngsi/Metadata_test.cpp                                         (haderding/remove_ngsiv1_indent: 2 render()
# o unittests/ngsi/NotifyConditionVector_test.cpp                            (haderding/remove_ngsiv1_indent: 3 render(), 3 check())
# o unittests/ngsi/NotifyCondition_test.cpp                                  (haderding/remove_ngsiv1_indent: 1 render(), 2 check())
# o unittests/ngsi/Originator_test.cpp                                       (haderding/remove_ngsiv1_indent: 2 render(), 3 check())
# o unittests/ngsi/ProvidingApplication_test.cpp                             (haderding/remove_ngsiv1_indent: 2 render()
# o unittests/ngsi/Reference_test.cpp                                        (haderding/remove_ngsiv1_indent: 2 render(), 1 check() + 1 var)
# o unittests/ngsi/RestrictionString_test.cpp                                (haderding/remove_ngsiv1_indent: 2 render(), 3 check())
# o unittests/ngsi/Restriction_test.cpp                                      (haderding/remove_ngsiv1_indent: 1 render(), 4 check())
# o unittests/ngsi/ScopeVector_test.cpp                                      (haderding/remove_ngsiv1_indent: 2 render(), 2 check())
# o unittests/ngsi/Scope_test.cpp                                            (haderding/remove_ngsiv1_indent: 1 render(), 4 check())
# o unittests/ngsi/StatusCode_test.cpp                                       (haderding/remove_ngsiv1_indent: 1 render(), 3 check())
# o unittests/ngsi/SubscribeError_test.cpp                                   (haderding/remove_ngsiv1_indent: 1 check())
# o unittests/ngsi/SubscriptionId_test.cpp                                   (haderding/remove_ngsiv1_indent: 3 check())
# o unittests/ngsi/Throttling_test.cpp                                       (haderding/remove_ngsiv1_indent: 3 render(), 3 check())
# o unittests/ngsi/UpdateActionType_test.cpp                                 (haderding/remove_ngsiv1_indent: 2 render()
#
# o unittests/ngsi10/NotifyContextRequest_test.cpp                           (haderding/remove_ngsiv1_indent: 4 render()
# o unittests/ngsi10/QueryContextRequest_test.cpp                            (haderding/remove_ngsiv1_indent: 1 render()
# o unittests/ngsi10/QueryContextResponse_test.cpp                           (haderding/remove_ngsiv1_indent: 14 render()
# o unittests/ngsi10/SubscribeContextResponse_test.cpp                       (haderding/remove_ngsiv1_indent: 6 render()
# o unittests/ngsi10/UnsubscribeContextRequest_test.cpp                      (haderding/remove_ngsiv1_indent: 1 render()
# o unittests/ngsi10/UnsubscribeContextResponse_test.cpp                     (haderding/remove_ngsiv1_indent: 2 render()
# o unittests/ngsi10/UpdateContextResponse_test.cpp                          (haderding/remove_ngsiv1_indent: 13 render()
# o unittests/ngsi10/UpdateContextSubscriptionRequest_test.cpp               (haderding/remove_ngsiv1_indent: 2 check())
# o unittests/ngsi10/UpdateContextSubscriptionResponse_test.cpp              (haderding/remove_ngsiv1_indent: 6 render()
#
# o unittests/ngsi9/DiscoverContextAvailabilityResponse_test.cpp             (haderding/remove_ngsiv1_indent: 34 render()
# o unittests/ngsi9/NotifyContextAvailabilityRequest_test.cpp                (haderding/remove_ngsiv1_indent: 3 render(), 1 check())
# o unittests/ngsi9/NotifyContextAvailabilityResponse_test.cpp               (haderding/remove_ngsiv1_indent: 1 render()
# o unittests/ngsi9/RegisterContextRequest_test.cpp                          (haderding/remove_ngsiv1_indent: 1 render()
# o unittests/ngsi9/RegisterContextResponse_test.cpp                         (haderding/remove_ngsiv1_indent: 4 render(), 1 check())
#
# o unittests/parse/CompoundValueNode_test.cpp                               (haderding/remove_ngsiv1_indent: 2 render()
# o unittests/parse/compoundValue_test.cpp                                   (haderding/remove_ngsiv1_indent: 1 render()
