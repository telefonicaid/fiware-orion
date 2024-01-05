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

#include "orionld/types/QNode.h"                                 // QNode
#include "orionld/types/OrionldRenderFormat.h"                   // OrionldRenderFormat
#include "orionld/common/orionldState.h"                         // orionldState
#include "orionld/common/orionldError.h"                         // orionldError
#include "orionld/common/numberToDate.h"                         // numberToDate
#include "orionld/context/orionldContextItemAliasLookup.h"       // orionldContextItemAliasLookup
#include "orionld/q/qAliasCompact.h"                             // qAliasCompact
#include "orionld/dbModel/dbModelValueStrip.h"                   // dbModelValueStrip
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
// notificationStatus -
//
static bool notificationStatus(KjNode* dbLastSuccessP, KjNode* dbLastFailureP)
{
  if ((dbLastSuccessP == NULL) && (dbLastFailureP == NULL))    return true;
  if ((dbLastSuccessP != NULL) && (dbLastFailureP == NULL))    return true;
  if ((dbLastSuccessP == NULL) && (dbLastFailureP != NULL))    return false;

  if (dbLastSuccessP->type != dbLastFailureP->type)
    return false;

  if (dbLastSuccessP->type == KjString)
  {
    if (strcmp(dbLastSuccessP->value.s, dbLastFailureP->value.s) < 0)
      return false;
  }
  else if (dbLastSuccessP->type == KjFloat)
  {
    if (dbLastSuccessP->value.f > dbLastFailureP->value.f)
      return true;
  }

  return true;
}



// -----------------------------------------------------------------------------
//
// dbModelToApiSubscription - modify the DB Model tree into an API Subscription
//
// PARAMETERS
//   dbSubP             KjNode tree of the subscription in database model
//   tenant             The tenant is needed for the subscription cache
//   forSubCache        When invoked for subscription cache, we want Floating points, not ISO8601 strings.
//                      We also want expanded names, not short names
//   qNodePP            Output QNode tree
//   coordinatesPP      Output coordinates tree
//   contextNodePP      Output @context node
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
//   "jsonldContext": "https://fiware.github.io/NGSI-LD_TestSuite/ldContext/testFullContext.jsonld"
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
KjNode* dbModelToApiSubscription
(
  KjNode*               dbSubP,
  const char*           tenant,
  bool                  forSubCache,
  QNode**               qNodePP,
  KjNode**              coordinatesPP,
  KjNode**              contextNodePP,
  KjNode**              showChangesP,
  KjNode**              sysAttrsP,
  OrionldRenderFormat*  renderFormatP,
  double*               timeIntervalP
)
{
  KjNode* dbSubIdP            = kjLookup(dbSubP, "_id");         DB_ITEM_NOT_FOUND(dbSubIdP, "id",          tenant);
  KjNode* dbNameP             = kjLookup(dbSubP, "name");
  KjNode* dbDescriptionP      = kjLookup(dbSubP, "description");
  KjNode* dbEntitiesP         = kjLookup(dbSubP, "entities");    DB_ITEM_NOT_FOUND(dbSubIdP, "entities",    tenant);
  KjNode* dbLdqP              = kjLookup(dbSubP, "ldQ");
  KjNode* qP                  = kjLookup(dbSubP, "q");
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
  KjNode* dbExpirationP       = kjLookup(dbSubP, "expiration");
  KjNode* dbLdContextP        = kjLookup(dbSubP, "ldContext");
  KjNode* dbCountP            = kjLookup(dbSubP, "count");
  KjNode* timesFailedP        = kjLookup(dbSubP, "timesFailed");
  KjNode* noMatchP            = kjLookup(dbSubP, "noMatch");
  KjNode* dbLastNotificationP = kjLookup(dbSubP, "lastNotification");
  KjNode* dbLastSuccessP      = kjLookup(dbSubP, "lastSuccess");
  KjNode* dbLastFailureP      = kjLookup(dbSubP, "lastFailure");
  KjNode* dbShowChangesP      = kjLookup(dbSubP, "showChanges");
  KjNode* dbSysAttrsP         = kjLookup(dbSubP, "sysAttrs");
  KjNode* dbLangP             = kjLookup(dbSubP, "lang");
  KjNode* dbCreatedAtP        = NULL;
  KjNode* dbModifiedAtP       = NULL;
  KjNode* timeIntervalNodeP   = kjLookup(dbSubP, "timeInterval");

  if ((orionldState.uriParamOptions.sysAttrs == true) || (forSubCache == true))
  {
    dbCreatedAtP  = kjLookup(dbSubP, "createdAt");   DB_ITEM_NOT_FOUND(dbSubIdP, "createdAt",   tenant);
    dbModifiedAtP = kjLookup(dbSubP, "modifiedAt");  DB_ITEM_NOT_FOUND(dbSubIdP, "modifiedAt",  tenant);
  }

  *contextNodePP = dbLdContextP;

  KjNode* apiSubP = kjObject(orionldState.kjsonP, NULL);

  // id
  dbSubIdP->name = (char*) "id";
  kjChildAdd(apiSubP, dbSubIdP);

  // type
  KjNode* typeNodeP = kjString(orionldState.kjsonP, "type", "Subscription");
  kjChildAdd(apiSubP, typeNodeP);

  // showChanges
  if ((dbShowChangesP != NULL) && (showChangesP != NULL))
    *showChangesP = dbShowChangesP;

  // sysAttrs
  if ((dbSysAttrsP != NULL) && (sysAttrsP != NULL))
    *sysAttrsP = dbSysAttrsP;

  //
  // If dbSubIdP is a JSON Object, it's an NGSIv2 subscription and its "id" looks like this:
  //   "id": { "$oid": "6290eafec8112b5716a931a7" }
  //
  // We need to extract "6290eafec8112b5716a931a7" and make dbSubIdP a KjString
  //
  if (dbSubIdP->type == KjObject)
  {
    KjNode* oidP = kjLookup(dbSubIdP, "$oid");

    if ((oidP == NULL) || (oidP->type != KjString))
      LM_RE(NULL, ("Un able to retrieve the subscription ID from what seems to be an NGSIv2 subscription"));

    dbSubIdP->type      = KjString;
    dbSubIdP->lastChild = NULL;
    dbSubIdP->value.s   = oidP->value.s;
  }

  //
  // tenant?
  //
  // Only if it's for the subscription cache
  //
  if ((forSubCache == true) && (tenant[0] != 0))
  {
    KjNode* tenantP = kjString(orionldState.kjsonP, "tenant", tenant);
    kjChildAdd(apiSubP, tenantP);
  }

  // name
  if (dbNameP != NULL)
  {
    dbNameP->name = (char*) "subscriptionName";
    kjChildAdd(apiSubP, dbNameP);
  }

  // description
  if (dbDescriptionP != NULL)
    kjChildAdd(apiSubP, dbDescriptionP);

  // lang
  if (dbLangP != NULL)
    kjChildAdd(apiSubP, dbLangP);

  //
  // "entities"
  //
  // Can be easily reused.
  // - isPattern needs to go, but if "true", then "id" => "idPattern" (if NGSi-LD subscription)
  // - isTypePattern is not supported by NGSI-LDS, but it is by NGSIv2 ... (removed if NGSi-LD subscription)
  //
  for (KjNode* entityP = dbEntitiesP->value.firstChildP; entityP != NULL; entityP = entityP->next)
  {
    KjNode* idP            = kjLookup(entityP, "id");
    KjNode* isPatternP     = kjLookup(entityP, "isPattern");
    KjNode* typeP          = kjLookup(entityP, "type");
    KjNode* isTypePatternP = kjLookup(entityP, "isTypePattern");

    // There is no "Type Pattern" in NGSI-LD
    kjChildRemove(entityP, isTypePatternP);

    // There is no "isPattern" in NGSI-LD
    kjChildRemove(entityP, isPatternP);

    if (strcmp(isPatternP->value.s, "true") == 0)
    {
      if (strcmp(idP->value.s, ".*") == 0)
        kjChildRemove(entityP, idP);
      else
        idP->name = (char*) "idPattern";
    }

    kjChildRemove(entityP, isPatternP);

    // type must be compacted
    // However, for sub-cache we need the long names
    if (forSubCache == false)
      typeP->value.s = orionldContextItemAliasLookup(orionldState.contextP, typeP->value.s, NULL, NULL);
  }
  kjChildAdd(apiSubP, dbEntitiesP);

  // watchedAttributes - NON-EMPTY !
  if ((dbConditionsP != NULL) && (dbConditionsP->value.firstChildP != NULL))
  {
    dbConditionsP->name = (char*) "watchedAttributes";
    kjChildAdd(apiSubP, dbConditionsP);

    // Now we need to go over all watched attributes and find their alias according to the current @context
    if (forSubCache == false)
    {
      for (KjNode* attrNameNodeP = dbConditionsP->value.firstChildP; attrNameNodeP != NULL; attrNameNodeP = attrNameNodeP->next)
      {
        attrNameNodeP->value.s = orionldContextItemAliasLookup(orionldState.contextP, attrNameNodeP->value.s, NULL, NULL);
      }
    }
  }

  // timeInterval
  if (timeIntervalNodeP != NULL)
  {
    kjChildAdd(apiSubP, timeIntervalNodeP);
    *timeIntervalP = (timeIntervalNodeP->type == KjInt)? timeIntervalNodeP->value.i : timeIntervalNodeP->value.f;
  }

  // "q" for NGSI-LD
  if ((dbLdqP != NULL) && (dbLdqP->value.s[0] != 0))
  {
    if (forSubCache == false)
    {
      dbModelValueStrip(dbLdqP);
      qAliasCompact(dbLdqP, true);
      dbLdqP->name = (char*) "q";
    }

    kjChildAdd(apiSubP, dbLdqP);
  }
  else if (qP != NULL)
  {
    if (forSubCache == false)
    {
      dbModelValueStrip(qP);
      qAliasCompact(qP, true);
    }

    kjChildAdd(apiSubP, qP);
  }

  // "geoQ"
  // The "expression" in the DB contains all 4 items of NGSI-LD's "geoQ".
  // It also contains "q" and "mq" of NGSIv2
  // So, let's just rename it to "geoQ", remove "q" amd "mq" if present (well, move them ...)
  //
  if (dbExpressionP != NULL)
  {
    KjNode* v2qP  = kjLookup(dbExpressionP, "q");
    KjNode* v2mqP = kjLookup(dbExpressionP, "mq");

    if (v2qP)
      kjChildRemove(dbExpressionP, v2qP);
    if (v2mqP)
      kjChildRemove(dbExpressionP, v2mqP);

    if (orionldState.apiVersion != API_VERSION_NGSILD_V1)  // FIXME: When taking from DB at startup, this won't work ...
    {
      if ((v2qP != NULL) && (v2qP->value.s[0] != 0))
        kjChildAdd(apiSubP, v2qP);

      if ((v2mqP != NULL) && (v2mqP->value.s[0] != 0))
        kjChildAdd(apiSubP, v2mqP);
    }

    bool empty = false;
    if (dbModelToApiGeoQ(dbExpressionP, coordinatesPP, &empty) == false)
    {
      // orionldError is done by dbModelToGeoQ
      return NULL;
    }

    if (empty == false)
    {
      dbExpressionP->name = (char*) "geoQ";
      kjChildAdd(apiSubP, dbExpressionP);
    }
  }

  // isActive (steal "status" if present)
  KjNode* isActiveP = NULL;
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

  // status
  KjNode* statusP;

  if (isActiveP != NULL)
  {
    if (isActiveP->value.b == true)
      statusP = kjString(orionldState.kjsonP, "status", "active");
    else
      statusP = kjString(orionldState.kjsonP, "status", "paused");
  }
  else
    statusP = kjString(orionldState.kjsonP, "status", "error");
  kjChildAdd(apiSubP, statusP);


  //
  // notification
  //
  KjNode* notificationP = kjObject(orionldState.kjsonP, "notification");
  if (dbAttrsP != NULL)
  {
    if (dbAttrsP->value.firstChildP != NULL)
    {
      dbAttrsP->name = (char*) "attributes";
      kjChildAdd(notificationP, dbAttrsP);

      // Find alias for all attributes
      for (KjNode* attrNameNodeP = dbAttrsP->value.firstChildP; attrNameNodeP != NULL; attrNameNodeP = attrNameNodeP->next)
      {
        attrNameNodeP->value.s = orionldContextItemAliasLookup(orionldState.contextP, attrNameNodeP->value.s, NULL, NULL);
      }
    }
  }

  if (dbFormatP != NULL)
  {
    kjChildAdd(notificationP, dbFormatP);
    *renderFormatP = renderFormat(dbFormatP->value.s);
  }

  KjNode* endpointP = kjObject(orionldState.kjsonP, "endpoint");

  kjChildAdd(notificationP, endpointP);

  // Notification::showChanges
  if ((dbShowChangesP != NULL) && (dbShowChangesP->value.b == true))
  {
    KjNode* showChangesP = kjBoolean(orionldState.kjsonP, "showChanges", true);
    kjChildAdd(notificationP, showChangesP);
  }

  // notification::sysAttrs
  if ((dbSysAttrsP != NULL) && (dbSysAttrsP->value.b == true))
  {
    KjNode* sysAttrsP = kjBoolean(orionldState.kjsonP, "sysAttrs", true);
    kjChildAdd(notificationP, sysAttrsP);
  }

  // notification::status
  bool    nStatus      = notificationStatus(dbLastSuccessP, dbLastFailureP);
  KjNode* nStatusNodeP = kjString(orionldState.kjsonP, "status", (nStatus == true)? "ok" : "failed");

  kjChildAdd(notificationP, nStatusNodeP);

  // lastNotification
  if (dbLastNotificationP != NULL)
    kjChildAdd(notificationP, dbLastNotificationP);

  // lastSuccess
  if (dbLastSuccessP != NULL)
    kjChildAdd(notificationP, dbLastSuccessP);

  // lastFailure
  if (dbLastFailureP != NULL)
    kjChildAdd(notificationP, dbLastFailureP);

  // timesSent
  if (dbCountP != NULL)
  {
    int timesSent = 0;

    if (dbCountP->type == KjInt)
      timesSent = dbCountP->value.i;
    else if (dbCountP->type == KjObject)
      LM_W(("Subscription::count: find the integer inside the object!"));

    //
    // Transform to KjInt named "timesSent"
    //
    dbCountP->name    = (char*) "timesSent";
    dbCountP->type    = KjInt;
    dbCountP->value.i = timesSent;

    kjChildAdd(notificationP, dbCountP);
    LM_T(LmtSubCacheStats, ("count/timesSent: %d", timesSent));
  }

  // timesFailed
  if (timesFailedP != NULL)
    kjChildAdd(notificationP, timesFailedP);

  // noMatch
  if (noMatchP != NULL)
    kjChildAdd(notificationP, noMatchP);

  // Add "notification" to top level
  kjChildAdd(apiSubP, notificationP);

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



  //
  // origin
  //
  KjNode* originP = kjString(orionldState.kjsonP, "origin", "database");
  kjChildAdd(apiSubP, originP);


  //
  // Headers
  //
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
    //
    // It's a double in the DB - need it to be an ISO8601 String in the API
    // UNLESS the user asks to get it as a Floating Point value
    //
    KjNode* expiresAtNodeP = NULL;
    if (forSubCache == false)
    {
      char dateTime[64];
      numberToDate(dbExpirationP->value.f, dateTime, sizeof(dateTime));

      expiresAtNodeP = kjString(orionldState.kjsonP, "expiresAt", dateTime);
    }
    else
      expiresAtNodeP = kjFloat(orionldState.kjsonP, "expiresAt", dbExpirationP->value.f);

    kjChildAdd(apiSubP, expiresAtNodeP);
  }

  // throttling
  if (dbThrottlingP != NULL)
  {
    // Not present if < 0
    if ((dbThrottlingP->type == KjInt) && (dbThrottlingP->value.i > 0))
      kjChildAdd(apiSubP, dbThrottlingP);
    if ((dbThrottlingP->type == KjFloat) && (dbThrottlingP->value.f > 0))
      kjChildAdd(apiSubP, dbThrottlingP);
  }

  // createdAt
  if (dbCreatedAtP != NULL)
  {
    //
    // It's a double in the DB - need it to be an ISO8601 String in the API
    // UNLESS the user asks to get it as a Floating Point value
    //
    KjNode* createdAtNodeP = NULL;
    if (forSubCache == false)
    {
      char dateTime[64];
      numberToDate(dbCreatedAtP->value.f, dateTime, sizeof(dateTime));

      createdAtNodeP = kjString(orionldState.kjsonP, "createdAt", dateTime);
    }
    else
      createdAtNodeP = kjFloat(orionldState.kjsonP, "createdAt", dbCreatedAtP->value.f);

    kjChildAdd(apiSubP, createdAtNodeP);
  }

  // modifiedAt
  if (dbModifiedAtP != NULL)
  {
    //
    // It's a double in the DB - need it to be an ISO8601 String in the API
    // UNLESS the user asks to get it as a Floating Point value
    //
    KjNode* modifiedAtNodeP = NULL;
    if (forSubCache == false)
    {
      char dateTime[64];
      numberToDate(dbModifiedAtP->value.f, dateTime, sizeof(dateTime));

      modifiedAtNodeP = kjString(orionldState.kjsonP, "modifiedAt", dateTime);
    }
    else
    {
      LM_T(LmtSubCacheSync, ("modifiedAt from DB: %f", dbModifiedAtP->value.f));
      modifiedAtNodeP = kjFloat(orionldState.kjsonP, "modifiedAt", dbModifiedAtP->value.f);
    }

    kjChildAdd(apiSubP, modifiedAtNodeP);
  }

  // jsonldContext
  if (dbLdContextP != NULL)
  {
    kjChildAdd(apiSubP, dbLdContextP);
    dbLdContextP->name = (char*) "jsonldContext";
  }


  if (qNodePP != NULL)  // FIXME: This is more than a bit weird ...
    *qNodePP = NULL;

  return apiSubP;
}
