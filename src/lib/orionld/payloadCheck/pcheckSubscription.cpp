/*
*
* Copyright 2019 FIWARE Foundation e.V.
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
}

#include "logMsg/logMsg.h"                                       // LM_*

#include "orionld/common/orionldState.h"                         // orionldState
#include "orionld/common/orionldError.h"                         // orionldError
#include "orionld/common/CHECK.h"                                // STRING_CHECK, ...
#include "orionld/q/qAliasCompact.h"                             // qAliasCompact
#include "orionld/context/orionldAttributeExpand.h"              // orionldAttributeExpand
#include "orionld/payloadCheck/fieldPaths.h"                     // Paths to fields in the payload, e.g. subscriptionNotification = Subscription::notification"
#include "orionld/payloadCheck/pcheckGeoQ.h"                     // pcheckGeoQ
#include "orionld/payloadCheck/pcheckEntityInfoArray.h"          // pcheckEntityInfoArray
#include "orionld/payloadCheck/pCheckNotification.h"             // pCheckNotification
#include "orionld/payloadCheck/pcheckSubscription.h"             // Own interface



// -----------------------------------------------------------------------------
//
// pcheckSubscription - need to merge with pCheckSubscription
//
// Used by orionldPatchSubscription()
//
bool pcheckSubscription
(
  KjNode*          subNodeP,
  bool             idCanBePresent,
  KjNode**         watchedAttributesPP,
  KjNode**         timeIntervalPP,
  KjNode**         qPP,
  KjNode**         geoqPP,
  KjNode**         geoCoordinatesPP,
  bool             patch,
  bool*            mqttChangeP
)
{
  KjNode* idP                     = NULL;
  KjNode* typeP                   = NULL;
  KjNode* nameP                   = NULL;
  KjNode* descriptionP            = NULL;
  KjNode* entitiesP               = NULL;
  KjNode* watchedAttributesP      = NULL;
  KjNode* timeIntervalP           = NULL;
  KjNode* qP                      = NULL;
  KjNode* geoqP                   = NULL;
  KjNode* csfP                    = NULL;
  KjNode* isActiveP               = NULL;
  KjNode* notificationP           = NULL;
  KjNode* expiresP                = NULL;
  KjNode* throttlingP             = NULL;
  KjNode* temporalqP              = NULL;
  KjNode* langP                   = NULL;
  int64_t dateTime;

  if (subNodeP->type != KjObject)
  {
    orionldError(OrionldBadRequestData, "Invalid Subscription", "The payload data for updating a subscription must be a JSON Object", 400);
    return false;
  }

  for (KjNode* nodeP = subNodeP->value.firstChildP; nodeP != NULL; nodeP = nodeP->next)
  {
    if (strcmp(nodeP->name, "id") == 0 || strcmp(nodeP->name, "@id") == 0)
    {
      if (idCanBePresent == false)
      {
        orionldError(OrionldBadRequestData, "The Subscription ID cannot be modified", "id", 400);
        return false;
      }

      DUPLICATE_CHECK(idP, "id", nodeP);
      STRING_CHECK(nodeP, nodeP->name);
      URI_CHECK(nodeP->value.s, nodeP->name, true);
    }
    else if (strcmp(nodeP->name, "type") == 0 || strcmp(nodeP->name, "@type") == 0)
    {
      DUPLICATE_CHECK(typeP, "type", nodeP);
      STRING_CHECK(nodeP, nodeP->name);

      if (strcmp(nodeP->value.s, "Subscription") != 0)
      {
        orionldError(OrionldBadRequestData, "Invalid value for Subscription Type", nodeP->value.s, 400);
        return false;
      }
    }
    else if (strcmp(nodeP->name, "name") == 0)
    {
      DUPLICATE_CHECK(nameP, "name", nodeP);
      STRING_CHECK(nodeP, nodeP->name);
      EMPTY_STRING_CHECK(nodeP, nodeP->name);
    }
    else if (strcmp(nodeP->name, "description") == 0)
    {
      DUPLICATE_CHECK(descriptionP, "description", nodeP);
      STRING_CHECK(nodeP, nodeP->name);
    }
    else if (strcmp(nodeP->name, "entities") == 0)
    {
      DUPLICATE_CHECK(entitiesP, "entities", nodeP);
      ARRAY_CHECK(entitiesP, "entities");
      EMPTY_ARRAY_CHECK(entitiesP, "entities");
      if (pcheckEntityInfoArray(entitiesP, true, false, SubscriptionEntitiesPathV) == false)
        return false;
    }
    else if (strcmp(nodeP->name, "watchedAttributes") == 0)
    {
      DUPLICATE_CHECK(watchedAttributesP, "watchedAttributes", nodeP);
      ARRAY_CHECK(nodeP, nodeP->name);
      EMPTY_ARRAY_CHECK(nodeP, nodeP->name);
      for (KjNode* itemP = nodeP->value.firstChildP; itemP != NULL; itemP = itemP->next)
      {
        STRING_CHECK(itemP, "watchedAttributes item");
        itemP->value.s = orionldAttributeExpand(orionldState.contextP, itemP->value.s, true, NULL);
      }
      *watchedAttributesPP = watchedAttributesP;
    }
    else if (strcmp(nodeP->name, "timeInterval") == 0)
    {
      DUPLICATE_CHECK(timeIntervalP, "timeInterval", nodeP);
      INTEGER_CHECK(nodeP, "timeInterval");
      *timeIntervalPP = timeIntervalP;
    }
    else if (strcmp(nodeP->name, "q") == 0)
    {
      DUPLICATE_CHECK(qP, "q", nodeP);
      STRING_CHECK(nodeP, "q");
      EMPTY_STRING_CHECK(nodeP, "q");
      *qPP = qP;
      qAliasCompact(qP, false);
    }
    else if (strcmp(nodeP->name, "geoQ") == 0)
    {
      DUPLICATE_CHECK(geoqP, "geoQ", nodeP);
      OBJECT_CHECK(nodeP, "geoQ");
      EMPTY_OBJECT_CHECK(nodeP, "geoQ");
      OrionldGeoInfo* geoInfoP = pcheckGeoQ(nodeP, true);
      if (geoInfoP == NULL)
        return false;
      *geoCoordinatesPP = geoInfoP->coordinates;
      *geoqPP = geoqP;
    }
    else if (strcmp(nodeP->name, "csf") == 0)
    {
      DUPLICATE_CHECK(csfP, "csf", nodeP);
      STRING_CHECK(nodeP, "csf");
      EMPTY_STRING_CHECK(nodeP, "csf");
    }
    else if (strcmp(nodeP->name, "isActive") == 0)
    {
      DUPLICATE_CHECK(isActiveP, "isActive", nodeP);
      BOOL_CHECK(nodeP, "isActive");
    }
    else if (strcmp(nodeP->name, "notification") == 0)
    {
      DUPLICATE_CHECK(notificationP, "notification", nodeP);
      OBJECT_CHECK(nodeP, "notification");
      EMPTY_OBJECT_CHECK(nodeP, "notification");
    }
    else if ((strcmp(nodeP->name, "expiresAt") == 0) || (strcmp(nodeP->name, "expires") == 0))
    {
      DUPLICATE_CHECK(expiresP, nodeP->name, nodeP);
      STRING_CHECK(nodeP, nodeP->name);
      EMPTY_STRING_CHECK(nodeP, nodeP->name);
      DATETIME_CHECK(expiresP->value.s, dateTime, nodeP->name);
    }
    else if (strcmp(nodeP->name, "lang") == 0)
    {
      DUPLICATE_CHECK(langP, nodeP->name, nodeP);
      STRING_CHECK(nodeP, nodeP->name);
      EMPTY_STRING_CHECK(nodeP, nodeP->name);
    }
    else if (strcmp(nodeP->name, "throttling") == 0)
    {
      DUPLICATE_CHECK(throttlingP, "throttling", nodeP);
      NUMBER_CHECK(nodeP, "throttling");
      POSITIVE_NUMBER_CHECK(nodeP, "throttling");
    }
    else if (strcmp(nodeP->name, "temporalQ") == 0)
    {
      DUPLICATE_CHECK(temporalqP, "temporalQ", nodeP);
      OBJECT_CHECK(nodeP, "temporalQ");
      EMPTY_OBJECT_CHECK(nodeP, "temporalQ");
    }
    else if (strcmp(nodeP->name, "status") == 0)
    {
      orionldError(OrionldBadRequestData, "Attempt to modify Read-Only attribute", "status", 400);
      return false;
    }
    else
    {
      orionldError(OrionldBadRequestData, "Unknown field in Subscription fragment", nodeP->name, 400);
      return false;
    }
  }

  KjNode* uriP;
  KjNode* notifierInfoP;

  if ((notificationP != NULL) && (pCheckNotification(notificationP, patch, &uriP, &notifierInfoP, mqttChangeP) == false))
      return false;

  return true;
}
