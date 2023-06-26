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
#include <map>                                                   // std::map (for httpInfo.headers)
#include <string>                                                // std::string (for httpInfo.headers iterator)

extern "C"
{
#include "kjson/KjNode.h"                                        // KjNode
#include "kjson/kjParse.h"                                       // kjParse
#include "kjson/kjBuilder.h"                                     // kjObject, kjString, ..., kjChildAdd
#include "kjson/kjClone.h"                                       // kjClone
}

#include "logMsg/logMsg.h"                                       // LM_*

#include "cache/subCache.h"                                      // CachedSubscription

#include "orionld/common/orionldState.h"                         // orionldState
#include "orionld/common/numberToDate.h"                         // numberToDate
#include "orionld/common/eqForDot.h"                             // eqForDot
#include "orionld/context/orionldContextItemAliasLookup.h"       // orionldContextItemAliasLookup
#include "orionld/dbModel/dbModelToApiCoordinates.h"             // dbModelToApiCoordinates
#include "orionld/dbModel/dbModelValueStrip.h"                   // dbModelValueStrip
#include "orionld/q/qAliasCompact.h"                             // qAliasCompact
#include "orionld/kjTree/kjTreeFromCachedSubscription.h"         // Own interface



// -----------------------------------------------------------------------------
//
// NULL_CHECK -
//
#define NULL_CHECK(pointer) if (pointer == NULL) return outOfMemory()



// -----------------------------------------------------------------------------
//
// outOfMemory -
//
static KjNode* outOfMemory(void)
{
  LM_E(("Internal Error (out of memory creating a KjNode-tree for a Subscription)"));
  return NULL;
}



// -----------------------------------------------------------------------------
//
// kjTreeFromCachedSubscription - in NGSI-LD API format
//
KjNode* kjTreeFromCachedSubscription(CachedSubscription* cSubP, bool sysAttrs, bool contextInBody)
{
  KjNode* nodeP;
  char    dateTime[64];

  //
  // Top level object - the Subscription
  //
  KjNode* sP = kjObject(orionldState.kjsonP, NULL);
  NULL_CHECK(sP);

  //
  // id
  //
  nodeP = kjString(orionldState.kjsonP, "id", cSubP->subscriptionId);
  NULL_CHECK(nodeP);
  kjChildAdd(sP, nodeP);

  //
  // type
  //
  nodeP = kjString(orionldState.kjsonP, "type", "Subscription");
  NULL_CHECK(nodeP);
  kjChildAdd(sP, nodeP);

  //
  // subscriptionName
  //
  if (cSubP->name != "")
  {
    nodeP = kjString(orionldState.kjsonP, "subscriptionName", cSubP->name.c_str());
    NULL_CHECK(nodeP);
    kjChildAdd(sP, nodeP);
  }

  //
  // description
  //
  if (cSubP->description != NULL)
  {
    nodeP = kjString(orionldState.kjsonP, "description", cSubP->description);
    NULL_CHECK(nodeP);
    kjChildAdd(sP, nodeP);
  }

  //
  // entities
  //
  int entities = cSubP->entityIdInfos.size();
  if (entities > 0)
  {
    KjNode* entitiesP = kjArray(orionldState.kjsonP, "entities");
    NULL_CHECK(entitiesP);

    for (int eIx = 0; eIx < entities; eIx++)
    {
      EntityInfo* eiP      = cSubP->entityIdInfos[eIx];
      KjNode*     eObjectP = kjObject(orionldState.kjsonP, NULL);
      NULL_CHECK(eObjectP);

      //
      // id/idPattern
      //
      // SPECIAL CASE:
      //  If it's a .* pattern, then it is omitted
      //
      const char*  entityId   = eiP->entityId.c_str();
      bool         noIdNeeded = (eiP->isPattern == true) && (strcmp(entityId, ".*") == 0);

      if ((noIdNeeded == false) && (*entityId != 0))
      {
        nodeP = (eiP->isPattern == false)? kjString(orionldState.kjsonP, "id", entityId) : kjString(orionldState.kjsonP, "idPattern", entityId);
        NULL_CHECK(nodeP);
        kjChildAdd(eObjectP, nodeP);
      }

      // type
      char* shortType = orionldContextItemAliasLookup(orionldState.contextP, eiP->entityType.c_str(), NULL, NULL);
      nodeP = kjString(orionldState.kjsonP, "type", shortType);
      NULL_CHECK(nodeP);

      kjChildAdd(eObjectP, nodeP);

      // Add the entitySelector object to the entities array
      kjChildAdd(entitiesP, eObjectP);
    }

    kjChildAdd(sP, entitiesP);
  }

  //
  // watchedAttributes
  //
  int watchedAttributes = (int) cSubP->notifyConditionV.size();
  if (watchedAttributes > 0)
  {
    KjNode* watchedAttributesP = kjArray(orionldState.kjsonP, "watchedAttributes");
    NULL_CHECK(watchedAttributesP);

    for (int ix = 0; ix < watchedAttributes; ix++)
    {
      char* alias = orionldContextItemAliasLookup(orionldState.contextP, cSubP->notifyConditionV[ix].c_str(), NULL, NULL);

      nodeP = kjString(orionldState.kjsonP, NULL, alias);
      NULL_CHECK(nodeP);
      kjChildAdd(watchedAttributesP, nodeP);
    }

    kjChildAdd(sP, watchedAttributesP);
  }

  //
  // NGSI-LD q
  //
  if (cSubP->qText != NULL)
  {
    nodeP = kjString(orionldState.kjsonP, "q", cSubP->qText);
    NULL_CHECK(nodeP);
    dbModelValueStrip(nodeP);
    qAliasCompact(nodeP, true);  // qAliasCompact uses orionldState.contextP - all OK
    kjChildAdd(sP, nodeP);
  }

  //
  //  geoQ
  //
  if (cSubP->expression.geometry != "")
  {
    KjNode* geoqP = kjObject(orionldState.kjsonP, "geoQ");
    NULL_CHECK(geoqP);

    nodeP = kjString(orionldState.kjsonP, "geometry", cSubP->expression.geometry.c_str());
    NULL_CHECK(nodeP);
    kjChildAdd(geoqP, nodeP);

    nodeP = kjString(orionldState.kjsonP, "georel", cSubP->expression.georel.c_str());
    NULL_CHECK(nodeP);
    kjChildAdd(geoqP, nodeP);

    nodeP = kjClone(orionldState.kjsonP, cSubP->geoCoordinatesP);
    nodeP->name = (char*) "coordinates";
    NULL_CHECK(nodeP);
    kjChildAdd(geoqP, nodeP);

    if (cSubP->expression.geoproperty != "")
    {
      // The geoproperty is encoded (dotForEq) - needs to be reversed before Alias-lookup
      char dotName[512];
      strncpy(dotName, cSubP->expression.geoproperty.c_str(), sizeof(dotName) - 1);
      eqForDot(dotName);
      char* shortName = orionldContextItemAliasLookup(orionldState.contextP, dotName, NULL, NULL);
      nodeP = kjString(orionldState.kjsonP, "geoproperty", shortName);
      NULL_CHECK(nodeP);
      kjChildAdd(geoqP, nodeP);
    }

    kjChildAdd(sP, geoqP);
  }

  //
  // status
  //
  if (cSubP->status == "active")
    nodeP = kjString(orionldState.kjsonP, "status", "active");
  else if (cSubP->status == "expired")
    nodeP = kjString(orionldState.kjsonP, "status", "expired");
  else
    nodeP = kjString(orionldState.kjsonP, "status", "paused");
  NULL_CHECK(nodeP);
  kjChildAdd(sP, nodeP);

  //
  // isActive - possibly setting it
  //
  if ((cSubP->expirationTime > 0) && (cSubP->expirationTime < orionldState.requestTime))
  {
    cSubP->isActive = false;
    cSubP->status   = "expired";
  }
  nodeP = kjBoolean(orionldState.kjsonP, "isActive", cSubP->isActive);
  NULL_CHECK(nodeP);
  kjChildAdd(sP, nodeP);

  //
  // notification
  //
  KjNode* notificationNodeP = kjObject(orionldState.kjsonP, "notification");
  NULL_CHECK(notificationNodeP);

  // notification::attributes
  int attrs = cSubP->attributes.size();
  if (attrs > 0)
  {
    KjNode* attributesNodeP = kjArray(orionldState.kjsonP, "attributes");

    for (int ix = 0; ix < attrs; ix++)
    {
      char* alias = orionldContextItemAliasLookup(orionldState.contextP, cSubP->attributes[ix].c_str(), NULL, NULL);
      nodeP = kjString(orionldState.kjsonP, NULL, alias);
      NULL_CHECK(nodeP);
      kjChildAdd(attributesNodeP, nodeP);
    }
    kjChildAdd(notificationNodeP, attributesNodeP);
  }

  // notification::format
  nodeP = kjString(orionldState.kjsonP, "format", renderFormatToString(cSubP->renderFormat));
  NULL_CHECK(nodeP);
  kjChildAdd(notificationNodeP, nodeP);

  // notification::showChanges
  if (cSubP->showChanges == true)
  {
    nodeP = kjBoolean(orionldState.kjsonP, "showChanges", true);
    NULL_CHECK(nodeP);
    kjChildAdd(notificationNodeP, nodeP);
  }

  // notification::sysAttrs
  if (cSubP->sysAttrs == true)
  {
    nodeP = kjBoolean(orionldState.kjsonP, "sysAttrs", true);
    NULL_CHECK(nodeP);
    kjChildAdd(notificationNodeP, nodeP);
  }

  // notification::endpoint
  KjNode* endpointNodeP = kjObject(orionldState.kjsonP, "endpoint");
  NULL_CHECK(endpointNodeP);

  // notification::endpoint::uri
  nodeP = kjString(orionldState.kjsonP, "uri", cSubP->httpInfo.url.c_str());
  NULL_CHECK(nodeP);
  kjChildAdd(endpointNodeP, nodeP);

  // notification::endpoint::accept
  nodeP = kjString(orionldState.kjsonP, "accept", mimeTypeToLongString(cSubP->httpInfo.mimeType));
  NULL_CHECK(nodeP);
  kjChildAdd(endpointNodeP, nodeP);

  // notification::endpoint::receiverInfo
  int ris = cSubP->httpInfo.headers.size();
  if (ris > 0)
  {
    KjNode* ri = kjArray(orionldState.kjsonP, "receiverInfo");
    NULL_CHECK(ri);

    for (std::map<std::string, std::string>::const_iterator it = cSubP->httpInfo.headers.begin(); it != cSubP->httpInfo.headers.end(); ++it)
    {
      const char* key   = it->first.c_str();
      const char* value = it->second.c_str();
      KjNode*     kvP   = kjObject(orionldState.kjsonP, NULL);
      NULL_CHECK(kvP);
      KjNode* keyP      = kjString(orionldState.kjsonP, "key",   key);
      NULL_CHECK(keyP);
      KjNode* valueP    = kjString(orionldState.kjsonP, "value", value);
      NULL_CHECK(valueP);

      kjChildAdd(kvP, keyP);
      kjChildAdd(kvP, valueP);
      kjChildAdd(ri, kvP);
    }

    kjChildAdd(endpointNodeP, ri);
  }

  // notification::endpoint::notifierInfo
  int nis = cSubP->httpInfo.notifierInfo.size();
  if (nis > 0)
  {
    KjNode* ni = kjArray(orionldState.kjsonP, "notifierInfo");
    NULL_CHECK(ni);

    for (int ix = 0; ix < nis; ix++)
    {
      KjNode* kvP  = kjObject(orionldState.kjsonP, NULL);
      NULL_CHECK(kvP);
      KjNode* keyP = kjString(orionldState.kjsonP, "key",   cSubP->httpInfo.notifierInfo[ix]->key);
      NULL_CHECK(keyP);
      KjNode* valueP = kjString(orionldState.kjsonP, "value", cSubP->httpInfo.notifierInfo[ix]->value);
      NULL_CHECK(valueP);

      kjChildAdd(kvP, keyP);
      kjChildAdd(kvP, valueP);
      kjChildAdd(ni, kvP);
    }
    kjChildAdd(endpointNodeP, ni);
  }
  kjChildAdd(notificationNodeP, endpointNodeP);

  //
  // notification::status
  //
  if (cSubP->consecutiveErrors == 0)
    nodeP = kjString(orionldState.kjsonP, "status", "ok");
  else
    nodeP = kjString(orionldState.kjsonP, "status", "failed");
  NULL_CHECK(nodeP);
  kjChildAdd(notificationNodeP, nodeP);

  //
  // notification::timesSent
  //
  if (cSubP->count + cSubP->dbCount > 0)
  {
    nodeP = kjInteger(orionldState.kjsonP, "timesSent", cSubP->count + cSubP->dbCount);
    NULL_CHECK(nodeP);
    kjChildAdd(notificationNodeP, nodeP);
  }

  //
  // notification::lastNotification
  //
  if (cSubP->lastNotificationTime > 0)
  {
    numberToDate(cSubP->lastNotificationTime, dateTime, sizeof(dateTime));
    nodeP = kjString(orionldState.kjsonP, "lastNotification", dateTime);
    NULL_CHECK(nodeP);
    kjChildAdd(notificationNodeP, nodeP);
  }

  //
  // notification::lastFailure
  //
  if (cSubP->lastFailure > 0)
  {
    numberToDate(cSubP->lastFailure, dateTime, sizeof(dateTime));
    nodeP = kjString(orionldState.kjsonP, "lastFailure", dateTime);
    NULL_CHECK(nodeP);
    kjChildAdd(notificationNodeP, nodeP);
  }

  //
  // notification::lastSuccess
  //
  if (cSubP->lastSuccess > 0)
  {
    numberToDate(cSubP->lastSuccess, dateTime, sizeof(dateTime));
    nodeP = kjString(orionldState.kjsonP, "lastSuccess", dateTime);
    NULL_CHECK(nodeP);
    kjChildAdd(notificationNodeP, nodeP);
  }

  //
  // notification::consecutiveErrors
  //
  if (cSubP->consecutiveErrors > 0)
  {
    nodeP = kjInteger(orionldState.kjsonP, "consecutiveErrors", cSubP->consecutiveErrors);
    NULL_CHECK(nodeP);
    kjChildAdd(notificationNodeP, nodeP);
  }

  //
  // notification::lastErrorReason
  //
  if (cSubP->lastErrorReason[0] != 0)
  {
    nodeP = kjString(orionldState.kjsonP, "lastErrorReason", cSubP->lastErrorReason);
    NULL_CHECK(nodeP);
    kjChildAdd(notificationNodeP, nodeP);
  }

  kjChildAdd(sP, notificationNodeP);

  //
  // expiresAt
  //
  if (cSubP->expirationTime > 0)
  {
    numberToDate(cSubP->expirationTime, dateTime, sizeof(dateTime));
    nodeP = kjString(orionldState.kjsonP, "expiresAt", dateTime);
    NULL_CHECK(nodeP);
    kjChildAdd(sP, nodeP);
  }

  //
  // throttling
  //
  if (cSubP->throttling > 0)
  {
    nodeP = kjFloat(orionldState.kjsonP, "throttling", cSubP->throttling);
    NULL_CHECK(nodeP);
    kjChildAdd(sP, nodeP);
  }

  // temporalQ - not implemented
  // scopeQ - not implemented

  //
  // lang
  //
  if (cSubP->lang != "")
  {
    nodeP = kjString(orionldState.kjsonP, "lang", cSubP->lang.c_str());
    NULL_CHECK(nodeP);
    kjChildAdd(sP, nodeP);
  }

  //
  // createdAt
  //
  if ((sysAttrs == true) && (cSubP->createdAt > 0))
  {
    numberToDate(cSubP->createdAt, dateTime, sizeof(dateTime));
    nodeP = kjString(orionldState.kjsonP, "createdAt", dateTime);
    NULL_CHECK(nodeP);
    kjChildAdd(sP, nodeP);
  }

  //
  // modifiedAt
  //
  if ((sysAttrs == true) && (cSubP->modifiedAt > 0))
  {
    numberToDate(cSubP->modifiedAt, dateTime, sizeof(dateTime));
    nodeP = kjString(orionldState.kjsonP, "modifiedAt", dateTime);
    NULL_CHECK(nodeP);
    kjChildAdd(sP, nodeP);
  }

  //
  // origin
  //
  if (experimental == true)
  {
    nodeP = kjString(orionldState.kjsonP, "origin", "cache");
    NULL_CHECK(nodeP);
    kjChildAdd(sP, nodeP);
  }

  //
  // @context
  //
  if (contextInBody == true)
  {
    nodeP = kjString(orionldState.kjsonP, "@context", orionldState.contextP->url);
    NULL_CHECK(nodeP);
    kjChildAdd(sP, nodeP);
  }

  return sP;
}
