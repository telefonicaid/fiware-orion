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
const char* SubscriptionIdPath                         = "Subscription::id";
const char* SubscriptionTypePath                       = "Subscription::type";
const char* SubscriptionNamePath                       = "Subscription::subscriptionName";
const char* SubscriptionDescriptionPath                = "Subscription::description";
const char* SubscriptionEntitiesPath                   = "Subscription::entities";
const char* SubscriptionEntitiesItemPath               = "Subscription::entities[X]";
const char* SubscriptionEntitiesIdPath                 = "Subscription::entities::id";
const char* SubscriptionEntitiesIdPatternPath          = "Subscription::entities::idPattern";
const char* SubscriptionEntitiesTypePath               = "Subscription::entities::type";
const char* SubscriptionWatchedAttributesPath          = "Subscription::watchedAttributes";
const char* SubscriptionWatchedAttributesItemPath      = "Subscription::watchedAttributes[X]";
const char* SubscriptionNotificationPath               = "Subscription::notification";
const char* SubscriptionNotificationFormatPath         = "Subscription::notification::format";
const char* SubscriptionNotificationAttributesPath     = "Subscription::notification::attributes";
const char* SubscriptionNotificationAttributesItemPath = "Subscription::notification::attributes[X]";
const char* SubscriptionNotificationEndpointPath       = "Subscription::notification::endpoint";
const char* SubscriptionNotificationShowChangesPath    = "Subscription::notification::showChanges";
const char* SubscriptionNotificationSysAttrsPath       = "Subscription::notification::sysAttrs";
const char* SubscriptionQPath                          = "Subscription::q";
const char* SubscriptionGeoqPath                       = "Subscription::geoQ";
const char* SubscriptionIsActivePath                   = "Subscription::isActive";
const char* SubscriptionExpiresAtPath                  = "Subscription::expiresAt";
const char* SubscriptionThrottlingPath                 = "Subscription::throttling";
const char* SubscriptionLangPath                       = "Subscription::lang";
const char* SubscriptionTemporalQPath                  = "Subscription::temporalQ";
const char* SubscriptionScopePath                      = "Subscription::scope";

const char* RegistrationInformationEntitiesPath               = "Registration::information[X]::entities";
const char* RegistrationInformationEntitiesItemPath           = "Registration::information[X]::entities[X]";
const char* RegistrationInformationEntitiesIdPath             = "Registration::information[X]::entities[X]::id";
const char* RegistrationInformationEntitiesIdPatternPath      = "Registration::information[X]::entities[X]::idPattern";
const char* RegistrationInformationEntitiesTypePath           = "Registration::information[X]::entities[X]::type";
const char* RegistrationInformationPropertyNamesPath          = "Registration::information[X]::propertyNames";
const char* RegistrationInformationPropertyNameItemPath       = "Registration::information[X]::propertyNames[X]";
const char* RegistrationInformationRelationshipNamesPath      = "Registration::information[X]::relationshipNames";
const char* RegistrationInformationRelationshipNamesItemPath  = "Registration::information[X]::relationshipNames[X]";


const char* PostQueryEntitiesPath            = "Query::entities";
const char* PostQueryEntitiesItemPath        = "Query::entities[X]";
const char* PostQueryEntitiesIdPath          = "Query::entities[X]::id";
const char* PostQueryEntitiesIdPatternPath   = "Query::entities[X]::idPattern";
const char* PostQueryEntitiesTypePath        = "Query::entities[X]::type";

const char* SubscriptionEntitiesPathV[] =
{
  SubscriptionEntitiesPath,
  SubscriptionEntitiesItemPath,
  SubscriptionEntitiesIdPath,
  SubscriptionEntitiesIdPatternPath,
  SubscriptionEntitiesTypePath
};

const char* RegistrationInformationEntitiesPathV[] =
{
  RegistrationInformationEntitiesPath,
  RegistrationInformationEntitiesItemPath,
  RegistrationInformationEntitiesIdPath,
  RegistrationInformationEntitiesIdPatternPath,
  RegistrationInformationEntitiesTypePath
};


const char* PostQueryEntitiesPathV[] =
{
  PostQueryEntitiesPath,
  PostQueryEntitiesItemPath,
  PostQueryEntitiesIdPath,
  PostQueryEntitiesIdPatternPath,
  PostQueryEntitiesTypePath
};
