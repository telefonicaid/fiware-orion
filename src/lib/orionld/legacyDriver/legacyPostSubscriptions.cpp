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

extern "C"
{
#include "kbase/kMacros.h"                                     // K_FT
#include "kalloc/kaStrdup.h"                                   // kaStrdup
#include "kjson/KjNode.h"                                      // KjNode
#include "kjson/kjLookup.h"                                    // kjLookup
#include "kjson/kjBuilder.h"                                   // kjString, kjChildAdd, ...
}

#include "logMsg/logMsg.h"                                     // LM_*

#include "apiTypesV2/Subscription.h"                           // Subscription
#include "rest/OrionError.h"                                   // OrionError
#include "mongoBackend/mongoGetSubscriptions.h"                // mongoGetLdSubscription
#include "mongoBackend/mongoCreateSubscription.h"              // mongoCreateSubscription

#include "orionld/common/orionldState.h"                       // orionldState, coreContextUrl
#include "orionld/common/orionldError.h"                       // orionldError
#include "orionld/http/httpHeaderLocationAdd.h"                // httpHeaderLocationAdd
#include "orionld/mqtt/mqttParse.h"                            // mqttParse
#include "orionld/mqtt/mqttConnectionEstablish.h"              // mqttConnectionEstablish
#include "orionld/legacyDriver/kjTreeToSubscription.h"         // kjTreeToSubscription
#include "orionld/legacyDriver/legacyPostSubscriptions.h"      // Own interface



// ----------------------------------------------------------------------------
//
// legacyPostSubscriptions -
//
bool legacyPostSubscriptions(void)
{
  ngsiv2::Subscription sub;
  std::string          subId;
  OrionError           oError;

  if (orionldState.contextP != NULL)
    sub.ldContext = orionldState.contextP->url;
  else
    sub.ldContext = coreContextUrl;

  //
  // FIXME: attrsFormat etc. should be set to default by constructor
  //        only ... there is no constructor ...
  //
  sub.attrsFormat         = RF_DEFAULT;
  sub.expires             = -1;  // 0?
  sub.throttling          = -1;  // 0?
  sub.timeInterval        = -1;  // 0?

  char*           subIdP        = NULL;
  KjNode*         endpointP     = NULL;
  bool            mqtt          = false;
  bool            mqtts         = false;
  char*           mqttUser      = NULL;
  char*           mqttPassword  = NULL;
  char*           mqttHost      = NULL;
  unsigned short  mqttPort      = 0;
  char*           mqttVersion   = NULL;
  char*           mqttTopic     = NULL;

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

    if ((strncmp(uriP->value.s, "mqtt://", 7) == 0) || (strncmp(uriP->value.s, "mqtts://", 8) == 0))
    {
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

      mqtt = true;
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
      orionldError(OrionldAlreadyExists, "A subscription with that ID already exists", subIdP, 409);
      return false;
    }
  }

  if (mqtt)
  {
    //
    // Establish connection with MQTT broker
    //
    if (mqttConnectionEstablish(mqtts, mqttUser, mqttPassword, mqttHost, mqttPort, mqttVersion) == false)
    {
      orionldError(OrionldInternalError, "Unable to connect to MQTT server", "xxx", 500);
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
  //
  // FIXME: Check oError for failure (oError is output from mongoCreateSubscription!)
  //        Disconnect from MQTT broker if needed

  if (subId == "")
  {
    LM_E(("Error creating subscription"));
    return false;
  }

  orionldState.httpStatusCode = 201;
  httpHeaderLocationAdd("/ngsi-ld/v1/subscriptions/", subId.c_str(), orionldState.tenantP->tenant);

  return true;
}
