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
#include <string>
#include <vector>

#include "logMsg/logMsg.h"                                     // LM_*

extern "C"
{
#include "kbase/kMacros.h"                                     // K_FT
#include "kalloc/kaStrdup.h"                                   // kaStrdup
#include "kjson/KjNode.h"                                      // KjNode
#include "kjson/kjLookup.h"                                    // kjLookup
#include "kjson/kjBuilder.h"                                   // kjString, kjChildAdd, ...
}

#include "common/globals.h"                                    // parse8601Time
#include "rest/OrionError.h"                                   // OrionError
#include "rest/httpHeaderAdd.h"                                // httpHeaderLocationAdd
#include "apiTypesV2/HttpInfo.h"                               // HttpInfo
#include "apiTypesV2/Subscription.h"                           // Subscription
#include "cache/subCache.h"                                    // subCacheItemLookup, CachedSubscription
#include "mongoBackend/mongoGetSubscriptions.h"                // mongoGetLdSubscription
#include "mongoBackend/mongoCreateSubscription.h"              // mongoCreateSubscription

#include "orionld/common/orionldState.h"                       // orionldState, coreContextUrl
#include "orionld/common/orionldError.h"                       // orionldError
#include "orionld/common/uuidGenerate.h"                       // uuidGenerate
#include "orionld/common/subCacheApiSubscriptionInsert.h"      // subCacheApiSubscriptionInsert
#include "orionld/kjTree/kjTreeToSubscription.h"               // kjTreeToSubscription
#include "orionld/kjTree/kjChildPrepend.h"                     // kjChildPrepend
#include "orionld/kjTree/kjTreeLog.h"                          // kjTreeLog
#include "orionld/dbModel/dbModelFromApiSubscription.h"        // dbModelFromApiSubscription
#include "orionld/mongoc/mongocSubscriptionExists.h"           // mongocSubscriptionExists
#include "orionld/mongoc/mongocSubscriptionInsert.h"           // mongocSubscriptionInsert
#include "orionld/mqtt/mqttParse.h"                            // mqttParse
#include "orionld/mqtt/mqttConnectionEstablish.h"              // mqttConnectionEstablish
#include "orionld/mqtt/mqttDisconnect.h"                       // mqttDisconnect
#include "orionld/q/QNode.h"                                   // QNode
#include "orionld/q/qRender.h"                                 // qRender
#include "orionld/q/qRelease.h"                                // qRelease
#include "orionld/q/qAliasCompact.h"                           // qAliasCompact
#include "orionld/payloadCheck/pCheckSubscription.h"           // pCheckSubscription
#include "orionld/serviceRoutines/orionldPostSubscriptions.h"  // Own Interface



// -----------------------------------------------------------------------------
//
// orionldPostSubscriptionsWithMongoBackend -
//
static bool orionldPostSubscriptionsWithMongoBackend(void)
{
  ngsiv2::Subscription sub;
  std::string          subId;
  OrionError           oError;

  LM_TMP(("KZ: In old POST Subscription function - using mongoBackend"));

  if (orionldState.contextP != NULL)
    sub.ldContext = orionldState.contextP->url;
  else
    sub.ldContext = coreContextUrl;

  //
  // FIXME: attrsFormat etc. should be set to default by constructor
  //        only ... there is no constructor ...
  //
  sub.attrsFormat = DEFAULT_RENDER_FORMAT;
  sub.expires             = -1;  // 0?
  sub.throttling          = -1;  // 0?
  sub.timeInterval        = -1;  // 0?

  char*    subIdP    = NULL;
  KjNode*  endpointP = NULL;

  // kjTreeToSubscription does the pCheckSubscription stuff ... for now ...
  if (kjTreeToSubscription(&sub, &subIdP, &endpointP) == false)
  {
    LM_E(("kjTreeToSubscription FAILED"));
    // orionldError is invoked by kjTreeToSubscription
    return false;
  }

  if (endpointP != NULL)
  {
    KjNode* uriP = kjLookup(endpointP, "uri");

    if (strncmp(uriP->value.s, "mqtt://", 7) == 0)
    {
      bool            mqtts         = false;
      char*           mqttUser      = NULL;
      char*           mqttPassword  = NULL;
      char*           mqttHost      = NULL;
      unsigned short  mqttPort      = 0;
      char*           mqttTopic     = NULL;
      char*           detail        = NULL;
      char*           uri           = kaStrdup(&orionldState.kalloc, uriP->value.s);  // Can't destroy uriP->value.s ... mqttParse is destructive!

      if (mqttParse(uri, &mqtts, &mqttUser, &mqttPassword, &mqttHost, &mqttPort, &mqttTopic, &detail) == false)
      {
        orionldError(OrionldBadRequestData, "Invalid MQTT endpoint", detail, 400);
        return false;
      }

      if (mqttUser     != NULL) strncpy(sub.notification.httpInfo.mqtt.username, mqttUser,     sizeof(sub.notification.httpInfo.mqtt.username) - 1);
      if (mqttPassword != NULL) strncpy(sub.notification.httpInfo.mqtt.password, mqttPassword, sizeof(sub.notification.httpInfo.mqtt.password) - 1);
      if (mqttHost     != NULL) strncpy(sub.notification.httpInfo.mqtt.host,     mqttHost,     sizeof(sub.notification.httpInfo.mqtt.host) - 1);
      if (mqttTopic    != NULL) strncpy(sub.notification.httpInfo.mqtt.topic,    mqttTopic,    sizeof(sub.notification.httpInfo.mqtt.topic) - 1);

      sub.notification.httpInfo.mqtt.mqtts = mqtts;
      sub.notification.httpInfo.mqtt.port  = mqttPort;

      //
      // Get MQTT-Version from notification:endpoint:notifierInfo Array, "key == MQTT-Version"
      //
      char*   mqttVersion   = NULL;
      int     mqttQoS;
      KjNode* notifierInfoP = kjLookup(endpointP, "notifierInfo");

      if (notifierInfoP)
      {
        for (KjNode* kvPairP = notifierInfoP->value.firstChildP; kvPairP != NULL; kvPairP = kvPairP->next)
        {
          KjNode* keyP   = kjLookup(kvPairP, "key");
          KjNode* valueP = kjLookup(kvPairP, "value");

          if ((keyP != NULL) && (valueP != NULL) && (keyP->type == KjString) && (valueP->type == KjString))
          {
            if (strcmp(keyP->name, "MQTT-Version") == 0)
            {
              mqttVersion = valueP->value.s;
              strncpy(sub.notification.httpInfo.mqtt.version, mqttVersion, sizeof(sub.notification.httpInfo.mqtt.version) - 1);
            }
            else if (strcmp(keyP->name, "MQTT-QoS") == 0)
            {
              mqttQoS = valueP->value.i;
              sub.notification.httpInfo.mqtt.qos = mqttQoS;
            }
          }
        }
      }


      //
      // Establish connection with MQTT broker
      //
      LM_TMP(("MQTT: mqtts:        %s", K_FT(mqtts)));
      LM_TMP(("MQTT: mqttUser:     %s", mqttUser));
      LM_TMP(("MQTT: mqttPassword: %s", mqttPassword));
      LM_TMP(("MQTT: mqttHost:     %s", mqttHost));
      LM_TMP(("MQTT: mqttPort:     %d", mqttPort));
      LM_TMP(("MQTT: mqttVersion:  %s", mqttVersion));
      if (mqttConnectionEstablish(mqtts, mqttUser, mqttPassword, mqttHost, mqttPort, mqttVersion) == false)
      {
        orionldError(OrionldInternalError, "Unable to connect to MQTT server", "xxx", 500);
        return false;
      }
    }
  }

  //
  // Does the subscription already exist?
  //
  // FIXME: Implement a function to ONLY check for existence - much faster
  //
  if (subIdP != NULL)
  {
    ngsiv2::Subscription      subscription;
    char*                     details;

    // mongoGetLdSubscription takes the req semaphore
    if (mongoGetLdSubscription(&subscription, subIdP, orionldState.tenantP, &orionldState.httpStatusCode, &details) == true)
    {
      orionldError(OrionldBadRequestData, "A subscription with that ID already exists", subIdP, 409);
      return false;
    }
  }

  //
  // Create the subscription
  //
  std::vector<std::string> servicePathV;
  servicePathV.push_back("/#");

  subId = mongoCreateSubscription(sub,
                                  &oError,
                                  orionldState.tenantP,
                                  servicePathV,
                                  orionldState.in.xAuthToken,
                                  orionldState.correlator,
                                  sub.ldContext,
                                  sub.lang);
  // FIXME: Check oError for failure (oError is output from mongoCreateSubscription!)

  orionldState.httpStatusCode = SccCreated;
  httpHeaderLocationAdd("/ngsi-ld/v1/subscriptions/", subId.c_str());

  return true;
}



// -----------------------------------------------------------------------------
//
// qFix
//
// 'q' ... one for Native NGSI-LD notifications, another one for NGSIv2 notifications
//
// If it turns out to be an 'mq' (q=A.B... => mq) for mongoBackend then an mq node is added to subP and
// dbModelFromApiSubscription moves it to its place.
//
// Either way, the NGSI-LD q will be on the toplevel as "ldQ"
//
bool qFix(KjNode* subP, KjNode* qNode, QNode* qTree)
{
  char qText[512];
  bool mq = false;

  LM_TMP(("KZ: qNode: '%s'", qNode->value.s));
  LM_TMP(("KZ: qTree: '%s'", qTree->value.s));

  //
  // The q-text might need to be re-written for NGSIv2, might even be an mq and not a q
  // The q-text might also be invalid for NGSIv2, in which case it is replaced by "P;!P" => no notifications
  //
  KjNode* qNodeForV2;
  KjNode* mqNodeForV2;

  if (qRender(qTree, V2, qText, sizeof(qText), &mq) == false)
  {
    qNodeForV2  = kjString(orionldState.kjsonP, "q", "P;!P");
    mqNodeForV2 = kjString(orionldState.kjsonP, "mq", "P.P;!P.P");
  }
  else if (mq == false)
  {
    qNodeForV2  = kjString(orionldState.kjsonP, "q", qText);
    mqNodeForV2 = kjString(orionldState.kjsonP, "mq", "P.P;!P.P");
  }
  else
  {
    qNodeForV2  = kjString(orionldState.kjsonP, "q", "P;!P");
    mqNodeForV2 = kjString(orionldState.kjsonP, "mq", qText);
  }

  qNode->name = (char*) "ldQ";  // The original "q" is taken for NGSI-LD

  // qNode (ldQ) is already in the tree - adding 'q' and 'mq'
  kjChildAdd(subP, qNodeForV2);
  kjChildAdd(subP, mqNodeForV2);

  LM_TMP(("KZ: %s: '%s'", qNode->name,       qNode->value.s));
  LM_TMP(("KZ: %s: '%s'", qNodeForV2->name,  qNodeForV2->value.s));
  LM_TMP(("KZ: %s: '%s'", mqNodeForV2->name, mqNodeForV2->value.s));
  return true;
}



// ----------------------------------------------------------------------------
//
// orionldPostSubscriptions -
//
bool orionldPostSubscriptions(void)
{
  if (experimental == false)
    return orionldPostSubscriptionsWithMongoBackend();  // this will be removed!! (after thorough testing)

  KjNode*  subP            = orionldState.requestTree;
  KjNode*  subIdP          = orionldState.payloadIdNode;
  KjNode*  endpointP       = NULL;
  KjNode*  qNode           = NULL;
  KjNode*  uriP            = NULL;
  KjNode*  notifierInfoP   = NULL;
  KjNode*  geoCoordinatesP = NULL;
  QNode*   qTree           = NULL;
  char*    qText           = NULL;
  char*    subId;
  bool     b;

  b = pCheckSubscription(subP,
                         orionldState.payloadIdNode,
                         orionldState.payloadTypeNode,
                         &endpointP,
                         &qNode,
                         &qTree,
                         &qText,
                         &uriP,
                         &notifierInfoP,
                         &geoCoordinatesP);
  if (b == false)
  {
    if (qTree != NULL)
      qRelease(qTree);

    return false;
  }

  if (subIdP != NULL)
  {
    subId = subIdP->value.s;

    // 'id' needs to be '_id' - mongo stuff ...
    subIdP->name = (char*) "_id";

    //
    // If the subscription already exists, a "409 Conflict" is returned
    //
    char* detail = NULL;
    if ((subCacheItemLookup(orionldState.tenantP->tenant, subId) != NULL) || (mongocSubscriptionExists(subId, &detail) == true))
    {
      if (detail == NULL)
        orionldError(OrionldAlreadyExists, "Subscription already exists", subId, 409);
      else
        orionldError(OrionldInternalError, "Database Error", detail, 500);

      if (qTree != NULL)
        qRelease(qTree);

      return false;
    }
  }
  else
  {
    char subscriptionId[80];
    strncpy(subscriptionId, "urn:ngsi-ld:subscription:", sizeof(subscriptionId) - 1);
    uuidGenerate(&subscriptionId[25], sizeof(subscriptionId) - 25, false);

    subIdP = kjString(orionldState.kjsonP, "_id", subscriptionId);
  }

  // Add subId to the tree
  kjChildPrepend(subP, subIdP);

  // The two 'q's ...
  if (qNode != NULL)
  {
    if (qFix(subP, qNode, qTree) == false)
    {
      if (qTree != NULL)
        qRelease(qTree);

      return false;
    }

    LM_TMP(("KZ: qNode: '%s': '%s'", qNode->name, qNode->value.s));
    qAliasCompact(qNode, false);
    LM_TMP(("KZ: qNode: '%s': '%s'", qNode->name, qNode->value.s));
  }

  // Timestamps
  KjNode* createdAt  = kjFloat(orionldState.kjsonP, "createdAt",  orionldState.requestTime);
  KjNode* modifiedAt = kjFloat(orionldState.kjsonP, "modifiedAt", orionldState.requestTime);

  kjChildAdd(subP, createdAt);
  kjChildAdd(subP, modifiedAt);

  // Counters ...


  //
  // If MQTT, the connection to the NQTT broker needs to be established before the subscription is accepted
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

  if (strncmp(uriP->value.s, "mqtt://", 7) == 0)  // pCheckSubscription guarantees uriP is non-NULL and a KjString
  {
    char*           detail        = NULL;
    char*           uri           = kaStrdup(&orionldState.kalloc, uriP->value.s);  // Can't destroy uriP->value.s ... mqttParse is destructive!

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
    LM_TMP(("MQTT: mqtts:        %s", K_FT(mqtts)));
    LM_TMP(("MQTT: mqttUser:     %s", mqttUser));
    LM_TMP(("MQTT: mqttPassword: %s", mqttPassword));
    LM_TMP(("MQTT: mqttHost:     %s", mqttHost));
    LM_TMP(("MQTT: mqttPort:     %d", mqttPort));
    LM_TMP(("MQTT: mqttVersion:  %s", mqttVersion));
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
  CachedSubscription* cSubP = subCacheApiSubscriptionInsert(subP, qTree, geoCoordinatesP, orionldState.contextP);

  // dbModel
  KjNode* dbSubscriptionP = subP;
  dbModelFromApiSubscription(dbSubscriptionP, false);

  // sub to db - mongocSubscriptionInsert(subP);
  if (mongocSubscriptionInsert(dbSubscriptionP, subIdP->value.s) == false)
  {
    // orionldError is done by mongocSubscriptionInsert
    LM_E(("mongocSubscriptionInsert failed"));
    if (mqttSubscription == true)
      mqttDisconnect(mqttHost, mqttPort, mqttUser, mqttPassword, mqttVersion);

    subCacheItemRemove(cSubP);

    if (qTree != NULL)
      qRelease(qTree);

    return false;
  }

  //
  // MQTT details to the cached subscription
  //
  bzero(&cSubP->httpInfo.mqtt.mqtts, sizeof(cSubP->httpInfo.mqtt.mqtts));
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

  orionldState.httpStatusCode = 201;
  httpHeaderLocationAdd("/ngsi-ld/v1/subscriptions/", subIdP->value.s);

  return true;
}
