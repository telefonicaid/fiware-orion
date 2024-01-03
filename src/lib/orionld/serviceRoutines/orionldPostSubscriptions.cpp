/*
*
* Copyright 2018 FIWARE Foundation e.V.
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
#include "kbase/kMacros.h"                                     // K_FT
#include "kalloc/kaStrdup.h"                                   // kaStrdup
#include "kjson/KjNode.h"                                      // KjNode
#include "kjson/kjLookup.h"                                    // kjLookup
#include "kjson/kjBuilder.h"                                   // kjString, kjChildAdd, ...
#include "kjson/kjClone.h"                                     // kjClone
}

#include "logMsg/logMsg.h"                                     // LM_*

#include "cache/subCache.h"                                    // subCacheItemLookup, CachedSubscription

#include "orionld/types/QNode.h"                               // QNode
#include "orionld/types/PernotSubscription.h"                  // PernotSubscription
#include "orionld/types/PernotSubCache.h"                      // PernotSubCache
#include "orionld/common/orionldState.h"                       // orionldState, coreContextUrl
#include "orionld/common/orionldError.h"                       // orionldError
#include "orionld/common/uuidGenerate.h"                       // uuidGenerate
#include "orionld/common/subCacheApiSubscriptionInsert.h"      // subCacheApiSubscriptionInsert
#include "orionld/http/httpHeaderLocationAdd.h"                // httpHeaderLocationAdd
#include "orionld/legacyDriver/legacyPostSubscriptions.h"      // legacyPostSubscriptions
#include "orionld/kjTree/kjChildPrepend.h"                     // kjChildPrepend
#include "orionld/dbModel/dbModelFromApiSubscription.h"        // dbModelFromApiSubscription
#include "orionld/mongoc/mongocSubscriptionExists.h"           // mongocSubscriptionExists
#include "orionld/mongoc/mongocSubscriptionInsert.h"           // mongocSubscriptionInsert
#include "orionld/pernot/pernotSubCacheAdd.h"                  // pernotSubCacheAdd
#include "orionld/pernot/pernotItemRelease.h"                  // pernotItemRelease
#include "orionld/pernot/pernotSubCacheLookup.h"               // pernotSubCacheLookup
#include "orionld/mqtt/mqttParse.h"                            // mqttParse
#include "orionld/mqtt/mqttConnectionEstablish.h"              // mqttConnectionEstablish
#include "orionld/mqtt/mqttDisconnect.h"                       // mqttDisconnect
#include "orionld/q/qRender.h"                                 // qRender
#include "orionld/q/qRelease.h"                                // qRelease
#include "orionld/q/qAliasCompact.h"                           // qAliasCompact
#include "orionld/q/qPresent.h"                                // qPresent
#include "orionld/payloadCheck/pCheckSubscription.h"           // pCheckSubscription
#include "orionld/serviceRoutines/orionldPostSubscriptions.h"  // Own Interface



// ----------------------------------------------------------------------------
//
// orionldPostSubscriptions -
//
bool orionldPostSubscriptions(void)
{
  if ((experimental == false) || (orionldState.in.legacy != NULL))
    return legacyPostSubscriptions();  // this will be removed!! (after thorough testing)

  KjNode*              subP            = orionldState.requestTree;
  KjNode*              subIdP          = orionldState.payloadIdNode;
  KjNode*              endpointP       = NULL;
  KjNode*              ldqNodeP        = NULL;
  KjNode*              uriP            = NULL;
  KjNode*              notifierInfoP   = NULL;
  KjNode*              geoCoordinatesP = NULL;
  QNode*               qTree           = NULL;
  char*                qRenderedForDb  = NULL;
  bool                 mqtt            = false;
  char*                subId           = NULL;
  bool                 b               = false;
  bool                 qValidForV2     = false;
  bool                 qIsMq           = false;
  KjNode*              showChangesP    = NULL;
  KjNode*              sysAttrsP       = NULL;
  double               timeInterval    = 0;
  OrionldRenderFormat  renderFormat    = RF_NORMALIZED;

  b = pCheckSubscription(subP,
                         true,
                         NULL,
                         orionldState.payloadIdNode,
                         orionldState.payloadTypeNode,
                         &endpointP,
                         &ldqNodeP,
                         &qTree,
                         &qRenderedForDb,
                         &qValidForV2,
                         &qIsMq,
                         &uriP,
                         &notifierInfoP,
                         &geoCoordinatesP,
                         &mqtt,
                         &showChangesP,
                         &sysAttrsP,
                         &timeInterval,
                         &renderFormat);

  if (qRenderedForDb != NULL)
    LM_T(LmtQ, ("qRenderedForDb: '%s'", qRenderedForDb));

  if (b == false)
  {
    if (qTree != NULL)
      qRelease(qTree);

    LM_RE(false, ("pCheckSubscription FAILED"));
  }

  // Subscription id special treats
  char subscriptionId[80];
  if (subIdP != NULL)
  {
    subId = subIdP->value.s;

    //
    // If the subscription already exists, a "409 Conflict" is returned
    //
    char* detail = NULL;
    if ((subCacheItemLookup(orionldState.tenantP->tenant, subId)   != NULL) ||
        (pernotSubCacheLookup(subId, orionldState.tenantP->tenant) != NULL) ||
        (mongocSubscriptionExists(subId, &detail)                  == true))
    {
      if (detail == NULL)
        orionldError(OrionldAlreadyExists, "Subscription already exists", subId, 409);
      else
        orionldError(OrionldInternalError, "Database Error", detail, 500);

      if (qTree != NULL)
        qRelease(qTree);

      return false;
    }

    strncpy(subscriptionId, subId, sizeof(subscriptionId) - 1);
  }
  else
  {
    char subscriptionId[80];
    uuidGenerate(subscriptionId, sizeof(subscriptionId), "urn:ngsi-ld:subscription:");
    subIdP = kjString(orionldState.kjsonP, "id", subscriptionId);
  }

  // Add subId to the tree
  kjChildPrepend(subP, subIdP);

  // The three 'q's ... that's also dbModel
  if (ldqNodeP != NULL)
  {
    ldqNodeP->name    = (char*) "ldQ";
    ldqNodeP->value.s = qRenderedForDb;

    // We robbed the "q" for "ldQ", need to add "q" and "mq" now - for NGSIv2
    KjNode* qNode;
    KjNode* mqNode;

    if (qValidForV2 == false)
    {
      qNode  = kjString(orionldState.kjsonP, "q", "P;!P");
      mqNode = kjString(orionldState.kjsonP, "mq", "P.P;!P.P");
      kjChildAdd(subP, qNode);
      kjChildAdd(subP, mqNode);
    }
    else if (qIsMq == false)
    {
      qNode  = kjString(orionldState.kjsonP, "q", qRenderedForDb);
      kjChildAdd(subP, qNode);
    }
    else
    {
      mqNode = kjString(orionldState.kjsonP, "mq", qRenderedForDb);
      kjChildAdd(subP, mqNode);
    }
  }

  // Timestamps
  KjNode* createdAt  = kjFloat(orionldState.kjsonP, "createdAt",  orionldState.requestTime);
  KjNode* modifiedAt = kjFloat(orionldState.kjsonP, "modifiedAt", orionldState.requestTime);

  kjChildAdd(subP, createdAt);
  kjChildAdd(subP, modifiedAt);

  // Counters ...


  //
  // If MQTT, the connection to the MQTT broker needs to be established before the subscription is accepted
  //
  bool            mqttSubscription = false;
  bool            mqtts            = false;
  char*           mqttUser         = NULL;
  char*           mqttPassword     = NULL;
  char*           mqttHost         = NULL;
  unsigned short  mqttPort         = 0;
  char*           mqttTopic        = NULL;
  char*           mqttVersion      = NULL;  // NOTE, my (KZ) local mosquitto seems to only support "mqtt3.1.1"
  int             mqttQoS          = 0;

  if (mqtt == true)
  {
    if (timeInterval != 0)
    {
      orionldError(OrionldBadRequestData, "Not Implemented", "Notifications in MQTT for Periodic Notification Subscription", 501);
      return false;
    }

    char*  detail = NULL;
    char*  uri    = kaStrdup(&orionldState.kalloc, uriP->value.s);  // Can't destroy uriP->value.s ... mqttParse is destructive!

    if (mqttParse(uri, &mqtts, &mqttUser, &mqttPassword, &mqttHost, &mqttPort, &mqttTopic, &detail) == false)
    {
      orionldError(OrionldBadRequestData, "Invalid MQTT endpoint", detail, 400);
      return false;
    }

    //
    // Get MQTT Version  from Subscription::notification::endpoint::notifierInfo Array, "key == MQTT-Version"
    // Get MQTT QoS      from Subscription::notification::endpoint::notifierInfo Array, "key == MQTT-QoS"
    //
    // Validity of the values is verified in pcheckNotifierInfo
    //
    if (notifierInfoP)
    {
      for (KjNode* kvPairP = notifierInfoP->value.firstChildP; kvPairP != NULL; kvPairP = kvPairP->next)
      {
        KjNode* keyP   = kjLookup(kvPairP, "key");
        KjNode* valueP = kjLookup(kvPairP, "value");

        if      (strcmp(keyP->name, "MQTT-Version") == 0)  mqttVersion = valueP->value.s;
        else if (strcmp(keyP->name, "MQTT-QoS")     == 0)  mqttQoS     = atoi(valueP->value.s);
      }
    }

    //
    // Establish connection with MQTT broker
    //
    if (mqttConnectionEstablish(mqtts, mqttUser, mqttPassword, mqttHost, mqttPort, mqttVersion) == false)
    {
      orionldError(OrionldInternalError, "Unable to connect to MQTT server", "xxx", 500);

      if (qTree != NULL)
        qRelease(qTree);

      return false;
    }

    mqttSubscription = true;
  }

  // sub to cache - BEFORE we change the tree to be according to the DB Model (as the DB model might change some day ...)
  CachedSubscription* cSubP = NULL;
  PernotSubscription* pSubP = NULL;

  if (timeInterval == 0)
  {
    cSubP = subCacheApiSubscriptionInsert(subP,
                                          qTree,
                                          geoCoordinatesP,
                                          orionldState.contextP,
                                          orionldState.tenantP->tenant,
                                          showChangesP,
                                          sysAttrsP,
                                          renderFormat);
  }
  else
  {
    // Add subscription to the pernot-cache
    LM_T(LmtPernot, ("qRenderedForDb: '%s'", qRenderedForDb));
    if (qTree != NULL)
      qPresent(qTree, "Pernot", "Q for pernot subscription", LmtPernot);
    pSubP = pernotSubCacheAdd(subscriptionId,
                              subP,
                              endpointP,
                              qTree,
                              geoCoordinatesP,
                              orionldState.contextP,
                              orionldState.tenantP,
                              showChangesP,
                              sysAttrsP,
                              renderFormat,
                              timeInterval);

    // Signal that there's a new Pernot subscription in the cache
    // ++pernotSubCache.newSubs;
    // LM_T(LmtPernotLoop, ("pernotSubCache.newSubs == %d", pernotSubCache.newSubs));
  }

  // dbModel
  KjNode* dbSubscriptionP = subP;
  subIdP->name = (char*) "_id";  // 'id' needs to be '_id' - mongo stuff ...
  dbModelFromApiSubscription(dbSubscriptionP, false);

  // sub to db - mongocSubscriptionInsert(subP);
  if (mongocSubscriptionInsert(dbSubscriptionP, subIdP->value.s) == false)
  {
    // orionldError is done by mongocSubscriptionInsert
    LM_E(("mongocSubscriptionInsert failed"));
    if (mqttSubscription == true)
      mqttDisconnect(mqttHost, mqttPort, mqttUser, mqttPassword, mqttVersion);

    if (cSubP != NULL)
      subCacheItemRemove(cSubP);
    else
      pernotItemRelease(pSubP);

    if (qTree != NULL)
      qRelease(qTree);

    return false;
  }

  //
  // MQTT details of the cached subscription
  // - For now, timeInterval cannot be done via MQTT
  //
  if (timeInterval == 0)
  {
    bzero(&cSubP->httpInfo.mqtt, sizeof(cSubP->httpInfo.mqtt));
    if (mqttSubscription == true)
    {
      cSubP->httpInfo.mqtt.mqtts = mqtts;
      cSubP->httpInfo.mqtt.port  = mqttPort;
      cSubP->httpInfo.mqtt.qos   = mqttQoS;

      if (mqttHost     != NULL)  strncpy(cSubP->httpInfo.mqtt.host,     mqttHost,     sizeof(cSubP->httpInfo.mqtt.host)     - 1);
      if (mqttUser     != NULL)  strncpy(cSubP->httpInfo.mqtt.username, mqttUser,     sizeof(cSubP->httpInfo.mqtt.username) - 1);
      if (mqttPassword != NULL)  strncpy(cSubP->httpInfo.mqtt.password, mqttPassword, sizeof(cSubP->httpInfo.mqtt.password) - 1);
      if (mqttVersion  != NULL)  strncpy(cSubP->httpInfo.mqtt.version,  mqttVersion,  sizeof(cSubP->httpInfo.mqtt.version)  - 1);
      if (mqttTopic    != NULL)  strncpy(cSubP->httpInfo.mqtt.topic,    mqttTopic,    sizeof(cSubP->httpInfo.mqtt.topic)    - 1);
    }
  }
  else if (mqttSubscription == true)
  {
    // This is already taken care of.
  }

  orionldState.httpStatusCode = 201;
  httpHeaderLocationAdd("/ngsi-ld/v1/subscriptions/", subIdP->value.s, orionldState.tenantP->tenant);

  return true;
}
