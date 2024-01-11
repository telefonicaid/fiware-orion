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
#include <string.h>                                              // strdup

#include <string>                                                // std::string, due to cSubP->expression.stringFilter.parse()

extern "C"
{
#include "kbase/kTime.h"                                         // kTimeGet
#include "kbase/kMacros.h"                                       // K_FT
#include "kjson/KjNode.h"                                        // kjBufferCreate
#include "kjson/kjLookup.h"                                      // kjLookup
#include "kjson/kjRender.h"                                      // kjFastRender
#include "kjson/kjClone.h"                                       // kjClone
}

#include "logMsg/logMsg.h"                                       // LM_*

#include "cache/CachedSubscription.h"                            // CachedSubscription
#include "cache/subCache.h"                                      // subCacheItemInsert

#include "orionld/types/QNode.h"                                 // QNode
#include "orionld/types/OrionldContext.h"                        // OrionldContext
#include "orionld/types/OrionldRenderFormat.h"                   // OrionldRenderFormat
#include "orionld/types/OrionldMimeType.h"                       // mimeTypeFromString
#include "orionld/dbModel/dbModelToApiCoordinates.h"             // dbModelToApiCoordinates
#include "orionld/mqtt/mqttParse.h"                              // mqttParse
#include "orionld/common/urlParse.h"                             // urlParse
#include "orionld/common/orionldState.h"                         // orionldState
#include "orionld/common/dateTime.h"                             // dateTimeFromString
#include "orionld/common/subCacheApiSubscriptionInsert.h"        // Own interface



// -----------------------------------------------------------------------------
//
// subCacheItemFill -
//
static void subCacheItemFill
(
  CachedSubscription*  cSubP,
  KjNode*              apiSubscriptionP,
  QNode*               qTree,
  KjNode*              geoCoordinatesP,
  OrionldContext*      contextP,
  const char*          tenant,
  KjNode*              showChangesP,
  KjNode*              sysAttrsP,
  OrionldRenderFormat  rFormat
)
{
  kjTreeLog(apiSubscriptionP, "apiSubscriptionP", LmtSubCacheSync);

  cSubP->tenant              = (tenant == NULL || *tenant == 0)? NULL : strdup(tenant);
  cSubP->servicePath         = strdup("/#");
  cSubP->qP                  = qTree;
  cSubP->contextP            = contextP;        // Right now, this is orionldState.contextP, i.e., the @context used when creating, except if "refresh"!
  cSubP->ldContext           = (contextP != NULL)? contextP->url : "";
  cSubP->geoCoordinatesP     = NULL;
  cSubP->showChanges         = (showChangesP != NULL)? showChangesP->value.b : false;
  cSubP->sysAttrs            = (sysAttrsP != NULL)? sysAttrsP->value.b : false;
  cSubP->renderFormat        = rFormat;

  LM_T(LmtSysAttrs, ("sysAttrs: %s", (cSubP->sysAttrs == true)? "true" : "false"));

  KjNode* subscriptionIdP    = kjLookup(apiSubscriptionP, "_id");  // "id" was changed to "_id" by orionldPostSubscriptions to accomodate the DB insertion
  KjNode* subscriptionNameP  = kjLookup(apiSubscriptionP, "subscriptionName");  // "name" is accepted too ...
  KjNode* descriptionP       = kjLookup(apiSubscriptionP, "description");
  KjNode* entitiesP          = kjLookup(apiSubscriptionP, "entities");
  KjNode* watchedAttributesP = kjLookup(apiSubscriptionP, "watchedAttributes");
  KjNode* qP                 = kjLookup(apiSubscriptionP, "q");
  KjNode* mqP                = kjLookup(apiSubscriptionP, "mq");
  KjNode* ldqP               = kjLookup(apiSubscriptionP, "ldQ");
  KjNode* geoqP              = kjLookup(apiSubscriptionP, "geoQ");
  KjNode* isActiveP          = kjLookup(apiSubscriptionP, "isActive");
  KjNode* notificationP      = kjLookup(apiSubscriptionP, "notification");
  KjNode* expiresAtP         = kjLookup(apiSubscriptionP, "expiresAt");
  KjNode* throttlingP        = kjLookup(apiSubscriptionP, "throttling");
  KjNode* langP              = kjLookup(apiSubscriptionP, "lang");
  KjNode* createdAtP         = kjLookup(apiSubscriptionP, "createdAt");
  KjNode* modifiedAtP        = kjLookup(apiSubscriptionP, "modifiedAt");
  KjNode* dbCountP           = kjLookup(notificationP,    "timesSent");
  KjNode* dbFailuresP        = kjLookup(notificationP,    "timesFailed");
  KjNode* lastNotificationP  = kjLookup(notificationP,    "lastNotification");
  KjNode* lastSuccessP       = kjLookup(notificationP,    "lastSuccess");
  KjNode* lastFailureP       = kjLookup(notificationP,    "lastFailure");

  if (subscriptionIdP == NULL)
    subscriptionIdP = kjLookup(apiSubscriptionP, "id");

  if (subscriptionIdP != NULL)
    cSubP->subscriptionId = strdup(subscriptionIdP->value.s);

  if (subscriptionNameP == NULL)
    subscriptionNameP = kjLookup(apiSubscriptionP, "name");

  if (subscriptionNameP != NULL)
    cSubP->name = subscriptionNameP->value.s;

  if (descriptionP != NULL)
    cSubP->description = strdup(descriptionP->value.s);


  //
  // DB Counters
  //
  cSubP->dbCount    = (dbCountP    != NULL)? dbCountP->value.i    : 0;
  cSubP->dbFailures = (dbFailuresP != NULL)? dbFailuresP->value.i : 0;

  LM_T(LmtSubCacheSync, ("%s: dbFailures == %d", subscriptionIdP->value.s, cSubP->dbFailures));
  LM_T(LmtSubCacheSync, ("%s: count      == %d", subscriptionIdP->value.s, cSubP->count));
  LM_T(LmtSubCacheSync, ("%s: dbCount    == %d", subscriptionIdP->value.s, cSubP->dbCount));


  //
  // timestamps
  //
  if (lastNotificationP != NULL)   cSubP->lastNotificationTime = lastNotificationP->value.f;
  if (lastSuccessP      != NULL)   cSubP->lastSuccess          = lastSuccessP->value.f;
  if (lastFailureP      != NULL)   cSubP->lastFailure          = lastFailureP->value.f;


  // Fixing false alarms for q and mq
  if ((qP != NULL) && (qP->value.s != NULL) && (qP->value.s[0] == 0))
    qP = NULL;
  if ((mqP != NULL) && (mqP->value.s != NULL) && (mqP->value.s[0] == 0))
    mqP = NULL;

  if (qP != NULL)
    cSubP->expression.q = qP->value.s;

  if (mqP != NULL)
    cSubP->expression.mq = mqP->value.s;

  if (ldqP != NULL)
    cSubP->qText = strdup(ldqP->value.s);


  if (isActiveP != NULL)
  {
    cSubP->isActive = isActiveP->value.b;
    cSubP->status   = (isActiveP->value.b == false)? "inactive" : "active";
  }
  else
  {
    cSubP->status   = "active";
    cSubP->isActive = true;
  }

  if (expiresAtP != NULL)
  {
    // "expiresAt" has already been translated to double?
    double ts;
    char   errorString[256];

    if (expiresAtP->type == KjString)
      ts = dateTimeFromString(expiresAtP->value.s, errorString, sizeof(errorString));
    else
      ts = expiresAtP->value.f;

    cSubP->expirationTime = ts;

    if ((cSubP->expirationTime > 0) && (cSubP->expirationTime < orionldState.requestTime))
    {
      cSubP->status   = "expired";
      cSubP->isActive = false;
    }
  }
  else
    cSubP->expirationTime = -1;

  if (throttlingP != NULL)
    cSubP->throttling = (throttlingP->type == KjFloat)? throttlingP->value.f : throttlingP->value.i;

  if (langP != NULL)
    cSubP->lang = langP->value.s;

  if (geoqP != NULL)
  {
    KjNode* geometryP    = kjLookup(geoqP, "geometry");
    KjNode* georelP      = kjLookup(geoqP, "georel");
    KjNode* geopropertyP = kjLookup(geoqP, "geoproperty");

    if (geometryP != NULL)
      cSubP->expression.geometry = geometryP->value.s;

    if (georelP != NULL)
      cSubP->expression.georel = georelP->value.s;

    if (geopropertyP != NULL)
      cSubP->expression.geoproperty = geopropertyP->value.s;

    if (geoCoordinatesP == NULL)
      geoCoordinatesP = kjLookup(geoqP, "coords");

    if (geoCoordinatesP != NULL)
    {
      if (geoCoordinatesP->type == KjString)
        geoCoordinatesP = dbModelToApiCoordinates(geoCoordinatesP->value.s);

      char coords[1024];
      kjFastRender(geoCoordinatesP, coords);
      cSubP->expression.coords = coords;  // Not sure this is 100% correct format, but is it used? DB is used for Geo ...
    }
  }

  if (watchedAttributesP != NULL)
  {
    for (KjNode* watchedAttributeP = watchedAttributesP->value.firstChildP; watchedAttributeP != NULL; watchedAttributeP = watchedAttributeP->next)
    {
      cSubP->notifyConditionV.push_back(watchedAttributeP->value.s);
    }
  }

  if (entitiesP != NULL)
  {
    for (KjNode* eSelectorP = entitiesP->value.firstChildP; eSelectorP != NULL; eSelectorP = eSelectorP->next)
    {
      KjNode*       idP             = kjLookup(eSelectorP, "id");
      KjNode*       idPatternP      = kjLookup(eSelectorP, "idPattern");
      KjNode*       typeP           = kjLookup(eSelectorP, "type");
      char*         id              = (idP            != NULL)? idP->value.s : NULL;
      char*         idPattern       = (idPatternP     != NULL)? idPatternP->value.s : NULL;
      char*         type            = (typeP          != NULL)? typeP->value.s : NULL;
      char*         isPattern       = (char*) "false";

      LM_T(LmtSR, ("id:           '%s'", id));
      LM_T(LmtSR, ("idPattern:    '%s'", idPattern));
      LM_T(LmtSR, ("type:         '%s'", type));

      if ((idP == NULL) && (idPattern == NULL))
      {
        id        = (char*) ".*";
        isPattern = (char*) "true";
      }
      else
      {
        if ((idP != NULL) && (idPatternP != NULL))
          idPatternP = NULL;

        if (idPattern != NULL)
        {
          id = idPatternP->value.s;
          isPattern = (char*) "true";
        }
      }

      LM_T(LmtSR, ("Creating a new EntityInfo (id: '%s', idPattern: '%s', type: '%s')", id, idPattern, type));
      EntityInfo* eP = new EntityInfo(id, type, isPattern, false);
      cSubP->entityIdInfos.push_back(eP);
    }
  }

  if (notificationP != NULL)
  {
    KjNode* attributesP = kjLookup(notificationP, "attributes");
    KjNode* formatP     = kjLookup(notificationP, "format");      // FIXME: pCheckSubscription has already found this field. Can be removed
    KjNode* endpointP   = kjLookup(notificationP, "endpoint");

    if (attributesP != NULL)
    {
      for (KjNode* attributeP = attributesP->value.firstChildP; attributeP != NULL; attributeP = attributeP->next)
      {
        cSubP->attributes.push_back(attributeP->value.s);
      }
    }

    // FIXME: pCheckSubscription has already found the "notification::format" field. Can be removed
    if (formatP != NULL)
      cSubP->renderFormat = renderFormat(formatP->value.s);

    if (cSubP->renderFormat == RF_NONE)
      cSubP->renderFormat = RF_NORMALIZED;

    if (endpointP != NULL)  // pCheckSubscription already ensures "endpoint" is present !!!
    {
      KjNode* uriP          = kjLookup(endpointP, "uri");
      KjNode* acceptP       = kjLookup(endpointP, "accept");
      KjNode* receiverInfoP = kjLookup(endpointP, "receiverInfo");
      KjNode* notifierInfoP = kjLookup(endpointP, "notifierInfo");

      if (uriP)  // pCheckSubscription already ensures "uri" is present !!!
      {
        cSubP->httpInfo.url = uriP->value.s;
        cSubP->url          = strdup(uriP->value.s);  // urlParse destroys the input
        urlParse(cSubP->url, &cSubP->protocolString, &cSubP->ip, &cSubP->port, &cSubP->rest);
        cSubP->protocol     = protocolFromString(cSubP->protocolString);
        LM_T(LmtAlt, ("Sub '%s'. protocol: '%s', IP: '%s', port: %d, rest: '%s'",
            cSubP->subscriptionId,
            cSubP->protocolString,
            cSubP->ip,
            cSubP->port,
            cSubP->rest));
        cSubP->protocolString = strdup(cSubP->protocolString);
        cSubP->ip             = strdup(cSubP->ip);
        cSubP->rest           = (cSubP->rest != NULL)? strdup(cSubP->rest) : NULL;
      }

      if (cSubP->protocol == MQTT)
      {
        char            url[512];
        bool            mqtts         = false;
        char*           mqttUser      = NULL;
        char*           mqttPassword  = NULL;
        char*           mqttHost      = NULL;
        unsigned short  mqttPort      = 0;
        char*           mqttTopic     = NULL;
        char*           detail        = NULL;

        strncpy(url, uriP->value.s, sizeof(url) - 1);
        if (mqttParse(url, &mqtts, &mqttUser, &mqttPassword, &mqttHost, &mqttPort, &mqttTopic, &detail) == false)
        {
          LM_E(("Internal Error (unable to parse mqtt URL)"));
          cSubP->isActive = false;
          return;
        }

        if (mqttUser     != NULL) strncpy(cSubP->httpInfo.mqtt.username, mqttUser,     sizeof(cSubP->httpInfo.mqtt.username) - 1);
        if (mqttPassword != NULL) strncpy(cSubP->httpInfo.mqtt.password, mqttPassword, sizeof(cSubP->httpInfo.mqtt.password) - 1);
        if (mqttHost     != NULL) strncpy(cSubP->httpInfo.mqtt.host,     mqttHost,     sizeof(cSubP->httpInfo.mqtt.host) - 1);
        if (mqttTopic    != NULL) strncpy(cSubP->httpInfo.mqtt.topic,    mqttTopic,    sizeof(cSubP->httpInfo.mqtt.topic) - 1);

        cSubP->httpInfo.mqtt.mqtts = mqtts;
        cSubP->httpInfo.mqtt.port  = mqttPort;
      }

      if (acceptP != NULL)
      {
        uint32_t acceptMask;
        cSubP->httpInfo.mimeType = mimeTypeFromString(acceptP->value.s, NULL, true, false, &acceptMask);
      }
      else
        cSubP->httpInfo.mimeType = MT_JSON;

      if (receiverInfoP != NULL)
      {
        for (KjNode* riP = receiverInfoP->value.firstChildP; riP != NULL; riP = riP->next)
        {
          KjNode* keyP   = kjLookup(riP, "key");
          KjNode* valueP = kjLookup(riP, "value");

          cSubP->httpInfo.headers[keyP->value.s] = valueP->value.s;
        }
      }

      if (notifierInfoP != NULL)
      {
        for (KjNode* niP = notifierInfoP->value.firstChildP; niP != NULL; niP = niP->next)
        {
          KjNode*   keyP   = kjLookup(niP, "key");
          KjNode*   valueP = kjLookup(niP, "value");
          KeyValue* kvP    = keyValueLookup(cSubP->httpInfo.notifierInfo, keyP->value.s);

          if (kvP != NULL)
            strncpy(kvP->value, valueP->value.s, sizeof(kvP->value) - 1);
          else
            keyValueAdd(&cSubP->httpInfo.notifierInfo, keyP->value.s, valueP->value.s);
        }
      }
    }
  }


  //
  // For legacy operations, we also need two Scopes filled:
  // 'q'
  // 'mq'
  //
  if (qP != NULL)
  {
    std::string errorString;
    if (cSubP->expression.stringFilter.parse(qP->value.s, &errorString) == false)
      LM_E(("Subscription '%s': invalid 'q': '%s'", cSubP->subscriptionId, qP->value.s));
  }

  if (mqP != NULL)
  {
    std::string errorString;
    if (cSubP->expression.mdStringFilter.parse(mqP->value.s, &errorString) == false)
      LM_E(("Subscription '%s': invalid 'mq': '%s'", cSubP->subscriptionId, mqP->value.s));
  }

  if (createdAtP != NULL)
    cSubP->createdAt = createdAtP->value.f;

  if (modifiedAtP != NULL)
    cSubP->modifiedAt = modifiedAtP->value.f;

  if (geoCoordinatesP != NULL)
    cSubP->geoCoordinatesP = kjClone(NULL, geoCoordinatesP);
}



// -----------------------------------------------------------------------------
//
// subCacheApiSubscriptionCreate -
//
static CachedSubscription* subCacheApiSubscriptionCreate
(
  KjNode*              apiSubscriptionP,
  QNode*               qTree,
  KjNode*              geoCoordinatesP,
  OrionldContext*      contextP,
  const char*          tenant,
  KjNode*              showChangesP,
  KjNode*              sysAttrsP,
  OrionldRenderFormat  renderFormat
)
{
  CachedSubscription* cSubP = new CachedSubscription();

  subCacheItemFill(cSubP, apiSubscriptionP, qTree, geoCoordinatesP, contextP, tenant, showChangesP, sysAttrsP, renderFormat);
  subCacheItemInsert(cSubP);
  return cSubP;
}



// -----------------------------------------------------------------------------
//
// subCacheApiSubscriptionUpdate -
//
static CachedSubscription* subCacheApiSubscriptionUpdate
(
  CachedSubscription*  cSubP,
  KjNode*              apiSubscriptionP,
  QNode*               qTree,
  KjNode*              geoCoordinatesP,
  OrionldContext*      contextP,
  const char*          tenant,
  KjNode*              showChangesP,
  KjNode*              sysAttrsP,
  OrionldRenderFormat  renderFormat
)
{
  LM_T(LmtSubCacheSync, ("Updating Cached Subscription (%p) from DB", cSubP));

  kjTreeLog(apiSubscriptionP, "apiSubscription", LmtSubCacheSync);

  KjNode* modifiedAtNode = kjLookup(apiSubscriptionP, "modifiedAt");
  if (modifiedAtNode == NULL)
  {
    struct timespec now;
    kTimeGet(&now);
    cSubP->modifiedAt = now.tv_sec + now.tv_nsec / 1000000000.0;
    LM_W(("Invalid subscription id DB - no 'modifiedAt' field"));
  }
  else
  {
    LM_T(LmtSubCacheSync, ("%s: modifiedAt in cache: %f", cSubP->subscriptionId, cSubP->modifiedAt));
    LM_T(LmtSubCacheSync, ("%s: modifiedAt in DB:    %f", cSubP->subscriptionId, modifiedAtNode->value.f));

    if (modifiedAtNode->value.f <= cSubP->modifiedAt)
    {
      LM_T(LmtSubCacheSync, ("%s: incoming is not newer - no change", cSubP->subscriptionId));
      return cSubP;
    }

    cSubP->modifiedAt = modifiedAtNode->value.f;  // The new value is in the DB
  }

  cSubP->count    = 0;
  cSubP->failures = 0;

  subCacheItemStrip(cSubP);
  subCacheItemFill(cSubP, apiSubscriptionP, qTree, geoCoordinatesP, contextP, tenant, showChangesP, sysAttrsP, renderFormat);

  return cSubP;
}



// -----------------------------------------------------------------------------
//
// subCacheApiSubscriptionInsert -
//
CachedSubscription* subCacheApiSubscriptionInsert
(
  KjNode*              apiSubscriptionP,
  QNode*               qTree,
  KjNode*              geoCoordinatesP,
  OrionldContext*      contextP,
  const char*          tenant,
  KjNode*              showChangesP,
  KjNode*              sysAttrsP,
  OrionldRenderFormat  renderFormat
)
{
  KjNode* subIdNodeP = kjLookup(apiSubscriptionP, "id");

  if (subIdNodeP == NULL)
    subIdNodeP = kjLookup(apiSubscriptionP, "_id");  // FIXME: always "id" !!!

  if (subIdNodeP == NULL)
  {
    kjTreeLog(apiSubscriptionP, "apiSubscriptionP", LmtSubCacheSync);
    LM_X(1, ("Subscription without id - exiting due to bug"));
  }

  char*               subId = subIdNodeP->value.s;
  CachedSubscription* subP  = subCacheItemLookup(tenant, subId);

  if (subP == NULL)
    return subCacheApiSubscriptionCreate(apiSubscriptionP, qTree, geoCoordinatesP, contextP, tenant, showChangesP, sysAttrsP, renderFormat);

  return subCacheApiSubscriptionUpdate(subP, apiSubscriptionP, qTree, geoCoordinatesP, contextP, tenant, showChangesP, sysAttrsP, renderFormat);
}
