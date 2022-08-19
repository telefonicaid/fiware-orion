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
    AttributeExpression.cpp
    ConditionValueList.cpp
    ContextAttribute.cpp
    ContextAttributeVector.cpp
    ContextElementResponse.cpp
    ContextElementResponseVector.cpp
    ContextRegistration.cpp
    ContextRegistrationVector.cpp
    ContextRegistrationAttribute.cpp
    ContextRegistrationAttributeVector.cpp
    ContextRegistrationResponse.cpp
    ContextRegistrationResponseVector.cpp
    Duration.cpp
    EntityId.cpp
    EntityIdVector.cpp
    Metadata.cpp
    MetadataVector.cpp
    NotifyCondition.cpp
    NotifyConditionVector.cpp
    Originator.cpp
    ProvidingApplication.cpp
    Reference.cpp
    RegistrationId.cpp
    Request.cpp
    Restriction.cpp
    RestrictionString.cpp
    Scope.cpp
    ScopeVector.cpp
    StatusCode.cpp
    StringList.cpp
    SubscribeResponse.cpp
    SubscribeError.cpp
    SubscriptionId.cpp
    Throttling.cpp
)

SET (HEADERS
    AttributeExpression.h
    ConditionValueList.h
    ContextAttribute.h
    ContextAttributeVector.h
    ContextElementResponse.h
    ContextElementResponseVector.h
    ContextRegistrationAttribute.h
    ContextRegistrationAttributeVector.h
    ContextRegistration.h
    ContextRegistrationVector.h
    ContextRegistrationResponse.h
    ContextRegistrationResponseVector.h
    Duration.h
    EntityId.h
    EntityIdVector.h
    Metadata.h
    MetadataVector.h
    NotifyCondition.h
    NotifyConditionVector.h
    Originator.h
    ProvidingApplication.h
    Reference.h
    RegistrationId.h
    Restriction.h
    RestrictionString.h
    Scope.h
    StatusCode.h
    StringList.h
    SubscribeError.h
    SubscribeResponse.h
    SubscriptionId.h
    Throttling.h
)



# Include directories
# -----------------------------------------------------------------
include_directories("${PROJECT_SOURCE_DIR}/src/lib")


# Library declaration
# -----------------------------------------------------------------
ADD_LIBRARY(ngsi STATIC ${SOURCES} ${HEADERS})
