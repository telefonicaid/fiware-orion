#ifndef SRC_LIB_ORIONLD_PAYLOADCHECK_FIELDPATHS_H_
#define SRC_LIB_ORIONLD_PAYLOADCHECK_FIELDPATHS_H_

/*
*
* Copyright 2022 FIWARE Foundation e.V.
*
* This file is part of Orion-LD Context Broker.
*
* Orion-LD Context Broker is free software: you can redistribute it and/or
* modify it under the terms of the GNU Affero General Public License as
* published by the Free Software Foundation, either version 3 of the
* License, or (at your option) any later version.
*
* Orion-LD Context Broker is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero
* General Public License for more details.
*
* You should have received a copy of the GNU Affero General Public License
* along with Orion-LD Context Broker. If not, see http://www.gnu.org/licenses/.
*
* For those usages not covered by this license please contact with
* orionld at fiware dot org
*
* Author: Ken Zangelin
*/



// -----------------------------------------------------------------------------
//
// To avoid unnecessary copies, variables to hold the API field paths
//
extern const char* SubscriptionIdPath;
extern const char* SubscriptionTypePath;
extern const char* SubscriptionNamePath;
extern const char* SubscriptionDescriptionPath;
extern const char* SubscriptionEntitiesPath;
extern const char* SubscriptionEntitiesItemPath;
extern const char* SubscriptionNotificationPath;
extern const char* SubscriptionNotificationFormatPath;
extern const char* SubscriptionNotificationAttributesPath;
extern const char* SubscriptionNotificationAttributesItemPath;
extern const char* SubscriptionNotificationEndpointPath;
extern const char* SubscriptionNotificationShowChangesPath;
extern const char* SubscriptionNotificationSysAttrsPath;
extern const char* SubscriptionWatchedAttributesPath;
extern const char* SubscriptionWatchedAttributesItemPath;
extern const char* SubscriptionQPath;
extern const char* SubscriptionGeoqPath;
extern const char* SubscriptionIsActivePath;
extern const char* SubscriptionExpiresAtPath;
extern const char* SubscriptionThrottlingPath;
extern const char* SubscriptionLangPath;
extern const char* SubscriptionTemporalQPath;
extern const char* SubscriptionScopePath;

extern const char* RegistrationInformationEntitiesPath;
extern const char* RegistrationInformationEntitiesItemPath;
extern const char* RegistrationInformationEntitiesIdPath;
extern const char* RegistrationInformationEntitiesIdPatternPath;
extern const char* RegistrationInformationEntitiesTypePath;
extern const char* RegistrationInformationPropertyNamesPath;
extern const char* RegistrationInformationPropertyNameItemPath;
extern const char* RegistrationInformationRelationshipNamesPath;
extern const char* RegistrationInformationRelationshipNamesItemPath;

extern const char* PostQueryEntitiesPath;
extern const char* PostQueryEntitiesItemPath;
extern const char* PostQueryEntitiesIdPath;
extern const char* PostQueryEntitiesIdPatternPath;
extern const char* PostQueryEntitiesTypePath;


//
// Arrays for functions shared between different services
//
extern const char* SubscriptionEntitiesPathV[];
extern const char* RegistrationInformationEntitiesPathV[];
extern const char* PostQueryEntitiesPathV[];

#endif  // SRC_LIB_ORIONLD_PAYLOADCHECK_FIELDPATHS_H_
