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
extern "C"
{
#include "kjson/KjNode.h"                                        // KjNode
#include "kjson/kjLookup.h"                                      // kjLookup
#include "kjson/kjBuilder.h"                                     // kjObject, kjString, kjChildAdd, ...
}

#include "logMsg/logMsg.h"                                       // LM_*

#include "orionld/common/orionldState.h"                         // orionldState
#include "orionld/common/orionldError.h"                         // orionldError
#include "orionld/q/QNode.h"                                     // QNode
#include "orionld/dbModel/dbModelToApiGeoQ.h"                    // dbModelToApiGeoQ
#include "orionld/dbModel/dbModelToApiSubscription.h"            // Own interface



// -----------------------------------------------------------------------------
//
// DB_ITEM_NOT_FOUND -
//
#define DB_ITEM_NOT_FOUND(pointer, fieldName, tenant)                                               \
do                                                                                                  \
{                                                                                                   \
  if (pointer == NULL)                                                                              \
  {                                                                                                 \
    LM_E(("Database Error (sub-cache item '%s' in tenant '%s' is not there)", fieldName, tenant));  \
    return NULL;                                                                                    \
  }                                                                                                 \
} while (0)



// -----------------------------------------------------------------------------
//
// dbModelToApiSubscription - modify the DB Model tree into an API Subscription
//
// Example of Subscription in DB:
// {
//   "_id": "urn:ngsi-ld:subs:S1",
//   "entities": [
//     {
//       "type": "http://example.org/T",
//       "id": ".*",
//       "isPattern": "true",
//       "isTypePattern": false
//     }
//   ],
//   "ldQ": "https://uri=etsi=org/ngsi-ld/default-context/temp.value<18"
//   "createdAt": 1653226155.598,
//   "modifiedAt": 1653226155.598,
//   "throttling": 0,
//   "expression": {
//     "geometry": "",
//     "coords": "",
//     "georel": "",
//     "geoproperty": "",
//     "q": "P;!P",
//     "mq": "P.P;!P.P"
//   },
//   "format": "simplified",
//   "reference": "http://127.0.0.1:9997/notify",
//   "mimeType": "application/json",
//   "attrs": [],
//   "conditions": [],
//   "status": "active",
//   "custom": false,
//   "servicePath": "/#",
//   "blacklist": false,
//   "ldContext": "https://fiware.github.io/NGSI-LD_TestSuite/ldContext/testFullContext.jsonld"
// }
//
// An API Subscription looks like this:
// {
//   "id": "urn:ngsi-ld:subscriptions:01",         => "id" => "_id"
//   "type": "Subscription",                       => Not in DB and not needed
//   "subscriptionName": "Test subscription S01",  => NEW and added to the datamodel ("name" is accepteb as well, and used for the DB - old name of the field)
//   "description": "XXX",                         => NEW and added to the datamodel
//   "entities": [                                 => SAME, but remember, "isTypePattern" : false (default value)
//     {
//       "id": "urn:ngsi-ld:E01",
//       "type": "T1"
//     }
//   ],
//   "watchedAttributes": [ "P2" ],           => "watchedAttributes" => "conditions"
//   "q": "P2>10",                            => "q" => "expression.q"
//   "geoQ": {                                => disappears: its children go into "expression"
//     "geometry": "Point",                   => "geoQ.geometry" => "expression.geometry"
//     "coordinates": [1,2],                  => "geoQ.coordinates" => "expression.coords"
//     "georel": "near;maxDistance==1",       => "geoQ.georel" => "expression.georel"
//     "geoproperty": "not supported"         => MUST BE ADDED: expression.geoproperty
//   },
//   "csf": "not implemented",                => NEW and added to the datamodel
//   "isActive": false,                       => "isActive" => "status" (== "inactive" or "active")
//   "notification": {                        => disappears: its children go into other fields
//     "attributes": [ "P1", "P2", "A3" ],    => "notification.attributes" => "attrs"
//     "format": "keyValues",                 => "notification.format" => "format"
//     "endpoint": {                          => disappears: its children go into other fields
//       "uri": "http://valid.url/url",       => "endpoint.uri" => "reference"
//       "accept": "application/ld+json"      => "endpoint.accept" => "mimeType"
//     }
//   },
//   "expiresAt": "2028-12-31T10:00:00",      => "expiresAt" => "expiration"
//   "throttling": 5                          => SAME
// }
//
KjNode* dbModelToApiSubscription(KjNode* dbSubP, const char* tenant, QNode** qNodePP, KjNode** coordinatesPP, KjNode** contextNodePP)
{
  KjNode* dbSubIdP            = kjLookup(dbSubP, "_id");         DB_ITEM_NOT_FOUND(dbSubIdP, "id",          tenant);
  KjNode* dbNameP             = kjLookup(dbSubP, "name");
  KjNode* dbDescriptionP      = kjLookup(dbSubP, "description");
  KjNode* dbEntitiesP         = kjLookup(dbSubP, "entities");    DB_ITEM_NOT_FOUND(dbSubIdP, "entities",    tenant);
  KjNode* dbLdqP              = kjLookup(dbSubP, "ldQ");
  KjNode* dbCreatedAtP        = kjLookup(dbSubP, "createdAt");   DB_ITEM_NOT_FOUND(dbSubIdP, "createdAt",   tenant);
  KjNode* dbModifiedAtP       = kjLookup(dbSubP, "modifiedAt");  DB_ITEM_NOT_FOUND(dbSubIdP, "modifiedAt",  tenant);
  KjNode* dbThrottlingP       = kjLookup(dbSubP, "throttling");  DB_ITEM_NOT_FOUND(dbSubIdP, "throttling",  tenant);
  KjNode* dbExpressionP       = kjLookup(dbSubP, "expression");  DB_ITEM_NOT_FOUND(dbSubIdP, "expression",  tenant);
  KjNode* dbFormatP           = kjLookup(dbSubP, "format");      DB_ITEM_NOT_FOUND(dbSubIdP, "format",      tenant);
  KjNode* dbReferenceP        = kjLookup(dbSubP, "reference");   DB_ITEM_NOT_FOUND(dbSubIdP, "reference",   tenant);
  KjNode* dbMimeTypeP         = kjLookup(dbSubP, "mimeType");    DB_ITEM_NOT_FOUND(dbSubIdP, "mimeType",    tenant);
  KjNode* dbHeadersP          = kjLookup(dbSubP, "headers");
  KjNode* dbNotifierInfoP     = kjLookup(dbSubP, "notifierInfo");
  KjNode* dbAttrsP            = kjLookup(dbSubP, "attrs");       DB_ITEM_NOT_FOUND(dbSubIdP, "attrs",       tenant);
  KjNode* dbConditionsP       = kjLookup(dbSubP, "conditions");  DB_ITEM_NOT_FOUND(dbSubIdP, "conditions",  tenant);
  KjNode* dbStatusP           = kjLookup(dbSubP, "status");
  KjNode* dbServicePathP      = kjLookup(dbSubP, "servicePath");
  KjNode* dbBlacklistP        = kjLookup(dbSubP, "blacklist");
  KjNode* dbExpirationP       = kjLookup(dbSubP, "expiration");
  KjNode* dbLdContextP        = kjLookup(dbSubP, "ldContext");
  KjNode* dbCountP            = kjLookup(dbSubP, "count");
  KjNode* dbLastNotificationP = kjLookup(dbSubP, "lastNotification");
  KjNode* dbLastSuccessP      = kjLookup(dbSubP, "lastSuccess");
  KjNode* dbLastFailureP      = kjLookup(dbSubP, "lastFailure");

  *contextNodePP = dbLdContextP;

  bool    ngsild  = (dbLdContextP != NULL);
  KjNode* apiSubP = kjObject(orionldState.kjsonP, NULL);

  // id
  dbSubIdP->name = (char*) "id";
  kjChildAdd(apiSubP, dbSubIdP);

  // tenant?
  KjNode* tenantP = kjString(orionldState.kjsonP, "tenant", tenant);
  kjChildAdd(apiSubP, tenantP);

  // name
  if (dbNameP != NULL)
  {
    dbNameP->name = (char*) "subscriptionName";
    kjChildAdd(apiSubP, dbNameP);
  }

  // description
  if (dbDescriptionP != NULL)
    kjChildAdd(apiSubP, dbDescriptionP);


  //
  // "entities"
  //
  // Can be easily reused.
  // - isPattern needs to go, but if "true", then "id" => "idPattern" (if NGSi-LD subscription)
  // - isTypePattern is not supported by NGSI-LDS, but it is by NGSIv2 ... (removed if NGSi-LD subscription)
  //
  for (KjNode* entityP = dbEntitiesP->value.firstChildP; entityP != NULL; entityP = entityP->next)
  {
    KjNode* isPatternP     = kjLookup(entityP, "isPattern");
    KjNode* isTypePatternP = kjLookup(entityP, "isTypePattern");

    if (ngsild)
    {
      if (isTypePatternP != NULL)
        kjChildRemove(entityP, isTypePatternP);

      if (isPatternP != NULL)
      {
        if (strcmp(isPatternP->value.s, "true") == 0)
        {
          KjNode* idP = kjLookup(entityP, "id");
          idP->name   = (char*) "idPattern";
        }
        kjChildRemove(entityP, isPatternP);
      }
    }
  }
  kjChildAdd(apiSubP, dbEntitiesP);

  // watchedAttributes
  if (dbConditionsP != NULL)
  {
    dbConditionsP->name = (char*) "watchedAttributes";
    kjChildAdd(apiSubP, dbConditionsP);
  }

  // "q" for NGSI-LD
  if (dbLdqP != NULL)
    kjChildAdd(apiSubP, dbLdqP);

  // "geoQ"
  // The "expression" in the DB contains all 4 items of NGSI-LD's "geoQ".
  // It also contains "q" and "mq" of NGSIv2
  // So, let's just rename it to "geoQ", remove "q" amd "mq" if present (well, move them ...)
  //
  KjNode* v2qP  = NULL;
  KjNode* v2mqP = NULL;
  if (dbExpressionP != NULL)
  {
    dbExpressionP->name = (char*) "geoQ";

    v2qP  = kjLookup(dbExpressionP, "q");
    if (v2qP != NULL)
    {
      kjChildRemove(dbExpressionP, v2qP);
      kjChildAdd(apiSubP, v2qP);
    }

    v2mqP = kjLookup(dbExpressionP, "mq");
    if (v2mqP != NULL)
    {
      kjChildRemove(dbExpressionP, v2mqP);
      kjChildAdd(apiSubP, v2mqP);
    }

    if (dbModelToApiGeoQ(dbExpressionP, coordinatesPP) == false)
    {
      // orionldError is done by dbModelToGeoQ
      return NULL;
    }

    kjChildAdd(apiSubP, dbExpressionP);
  }

  // isActive (steal "status" if present)
  KjNode* isActiveP;
  if (dbStatusP != NULL)
  {
    isActiveP = dbStatusP;
    isActiveP->name = (char*) "isActive";

    // In NGSIv2, "status" can take 2 values: "active", "inactive"
    if (strcmp(isActiveP->value.s, "inactive") == 0)
      isActiveP->value.b = false;
    else
      isActiveP->value.b = true;
    isActiveP->type = KjBoolean;
  }
  else
    isActiveP = kjBoolean(orionldState.kjsonP, "isActive", true);

  // FIXME: Check also "expiration"
  kjChildAdd(apiSubP, isActiveP);


  //
  // notification
  //
  KjNode* notificationP = kjObject(orionldState.kjsonP, "notification");
  if (dbAttrsP != NULL)
  {
    dbAttrsP->name = (char*) "attributes";
    kjChildAdd(notificationP, dbAttrsP);
  }
  if (dbFormatP != NULL)
    kjChildAdd(notificationP, dbFormatP);

  KjNode* endpointP = kjObject(orionldState.kjsonP, "endpoint");
  kjChildAdd(notificationP, endpointP);

  if (dbReferenceP != NULL)
  {
    kjChildAdd(endpointP, dbReferenceP);
    dbReferenceP->name = (char*) "uri";
  }

  if (dbMimeTypeP != NULL)
  {
    kjChildAdd(endpointP, dbMimeTypeP);
    dbMimeTypeP->name = (char*) "accept";
  }

  kjChildAdd(apiSubP, notificationP);


  if (dbHeadersP != NULL)
  {
    // In the database, "headers" is stored as:
    // {
    //   "key1": "value1",
    //   "key2": "value2"
    // }
    //
    // The API nees it to instead be like this:
    //
    // [
    //   {
    //     "key": "key1",
    //     "value": "value1"
    //   },
    //   {
    //     "key": "key2",
    //     "value": "value2"
    //   }
    // ]
    //
    // Same same for notifierInfo
    //
    KjNode* receiverInfoP = kjArray(orionldState.kjsonP, "receiverInfo");
    for (KjNode* kvP = dbHeadersP->value.firstChildP; kvP != NULL; kvP = kvP->next)
    {
      KjNode* objectP   = kjObject(orionldState.kjsonP, NULL);
      KjNode* keyP      = kjString(orionldState.kjsonP, "key", kvP->name);
      KjNode* valueP    = kjString(orionldState.kjsonP, "value", kvP->value.s);

      kjChildAdd(objectP, keyP);
      kjChildAdd(objectP, valueP);
      kjChildAdd(receiverInfoP, objectP);
    }
    kjChildAdd(endpointP, receiverInfoP);
  }

  if (dbNotifierInfoP != NULL)
  {
    KjNode* notifierInfoP = kjArray(orionldState.kjsonP, "notifierInfo");
    for (KjNode* kvP = dbNotifierInfoP->value.firstChildP; kvP != NULL; kvP = kvP->next)
    {
      KjNode* objectP   = kjObject(orionldState.kjsonP, NULL);
      KjNode* keyP      = kjString(orionldState.kjsonP, "key", kvP->name);
      KjNode* valueP    = kjString(orionldState.kjsonP, "value", kvP->value.s);

      kjChildAdd(objectP, keyP);
      kjChildAdd(objectP, valueP);
      kjChildAdd(notifierInfoP, objectP);
    }
    kjChildAdd(endpointP, notifierInfoP);
  }

  if (dbExpirationP != NULL)
  {
    kjChildAdd(apiSubP, dbExpirationP);
    dbExpirationP->name = (char*) "expiresAt";
  }

  // throttling
  if (dbThrottlingP != NULL)
    kjChildAdd(apiSubP, dbThrottlingP);

  // createdAt
  if (dbCreatedAtP != NULL)
    kjChildAdd(apiSubP, dbCreatedAtP);

  // modifiedAt
  if (dbModifiedAtP != NULL)
    kjChildAdd(apiSubP, dbModifiedAtP);

  // FIXME: custom
  // KjNode* dbCustomP = kjLookup(dbSubP, "custom");

  // servicePath
  if (dbServicePathP != NULL)
    kjChildAdd(apiSubP, dbServicePathP);

  // blacklist
  if (dbBlacklistP != NULL)
    kjChildAdd(apiSubP, dbBlacklistP);

  // ldContext
  if (dbLdContextP != NULL)
    kjChildAdd(apiSubP, dbLdContextP);

  // count
  if (dbCountP != NULL)
    kjChildAdd(apiSubP, dbCountP);

  // lastNotification
  if (dbLastNotificationP != NULL)
    kjChildAdd(apiSubP, dbLastNotificationP);

  // lastSuccess
  if (dbLastSuccessP != NULL)
    kjChildAdd(apiSubP, dbLastSuccessP);

  // lastFailure
  if (dbLastFailureP != NULL)
    kjChildAdd(apiSubP, dbLastFailureP);

  *qNodePP = NULL;

  return apiSubP;
}
