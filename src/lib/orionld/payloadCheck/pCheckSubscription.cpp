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
#include "kbase/kMacros.h"                                       // K_FT
#include "kjson/KjNode.h"                                        // KjNode
#include "kjson/kjBuilder.h"                                     // kjChildRemove
#include "kjson/kjLookup.h"                                      // kjLookup
}

#include "logMsg/logMsg.h"                                       // LM_*

#include "orionld/types/QNode.h"                                 // QNode
#include "orionld/types/OrionldRenderFormat.h"                   // OrionldRenderFormat
#include "orionld/q/qBuild.h"                                    // qBuild
#include "orionld/payloadCheck/PCHECK.h"                         // PCHECK_*
#include "orionld/payloadCheck/fieldPaths.h"                     // Paths to fields in the payload, e.g. subscriptionNotification = Subscription::notification"
#include "orionld/payloadCheck/pcheckEntityInfoArray.h"          // pcheckEntityInfoArray
#include "orionld/payloadCheck/pcheckGeoQ.h"                     // pcheckGeoQ
#include "orionld/payloadCheck/pCheckNotification.h"             // pCheckNotification
#include "orionld/payloadCheck/pCheckStringArray.h"              // pCheckStringArray
#include "orionld/payloadCheck/pCheckSubscription.h"             // Own interface



// -----------------------------------------------------------------------------
//
// pCheckSubscription -
//
// An NGSI-LD subscription contains the following fields:
// - id                         Not mandatory - the broker may invent it
// - type                       Not in DB (must be "Subscription" - will not be saved in mongo)
// - subscriptionName/name
// - description
// - entities[
//     {
//       id                    id takes precedence over idPattern
//       idPattern
//       type                  Mandatory
//     }
//   ]
// - watchedAttributes[String]
// - timeInterval               NOT SUPPORTED => 501 if present (will not be implemented any time soon - not at all useful)
// - q
// - geoQ {
//     geometry
//     coordinates[]
//     georel
//     geoproperty
//   }
// - csf                        That's for Context Registration Subscriptions - 501 if present
// - isActive                   true/false - set the subscription is PAUSED/ACTIVE mode
// - notification
//   {
//     attributes[]
//     format (normalized, concise, simplified/keyValues)
//     status   (String - read-only: "ok" or "failed") - ignored if present in create/update?
//     endpoint {
//       url             Mandatory URI
//       accept          "application/[json|ld+json|geo+json]
//       receiverInfo[]  key-value array with HTTP headers to be forwarded in notifs
//       notifierInfo[]  key-value array with info for MQTT connections (and future stuff)
//     }
//   }
// - expires/expiresAt
// - throttling
// - temporalQ                  That's for Context Registration Subscriptions - 501 if present
// - scopeQ
// - context                                                 Not yet in spec but is already agreed upon
// - lang
// - status
//
// * At least one of 'entities' and 'watchedAttributes' must be present.
// * Either 'timeInterval' or 'watchedAttributes' must be present. But not both of them
// * For now, 'timeInterval' will not be implemented. If ever ...
//
bool pCheckSubscription
(
  KjNode*               subP,
  bool                  isCreate,          // true if POST, false if PATCH
  char*                 subscriptionId,    // non-NULL if PATCH
  KjNode*               idP,
  KjNode*               typeP,
  KjNode**              endpointP,
  KjNode**              qNodeP,
  QNode**               qTreeP,
  char**                qRenderedForDbP,
  bool*                 qValidForV2P,
  bool*                 qIsMqP,
  KjNode**              uriPP,
  KjNode**              notifierInfoPP,
  KjNode**              geoCoordinatesPP,
  bool*                 mqttChangeP,
  KjNode**              showChangesP,
  KjNode**              sysAttrsP,
  double*               timeInterval,
  OrionldRenderFormat*  renderFormatP
)
{
  PCHECK_OBJECT(subP, 0, NULL, "A Subscription must be a JSON Object", 400);

  KjNode* nameP               = NULL;
  KjNode* descriptionP        = NULL;
  KjNode* entitiesP           = NULL;
  KjNode* watchedAttributesP  = NULL;
  KjNode* isActiveP           = NULL;
  KjNode* notificationP       = NULL;
  KjNode* expiresAtP          = NULL;
  KjNode* throttlingP         = NULL;
  KjNode* qP                  = NULL;
  KjNode* geoqP               = NULL;
  KjNode* langP               = NULL;
  KjNode* timeIntervalP       = NULL;
  double  expiresAt;

  *mqttChangeP  = NULL;
  *showChangesP = NULL;
  *sysAttrsP    = NULL;
  *timeInterval = 0;

  if (idP != NULL)
  {
    PCHECK_STRING(idP, 0, NULL, SubscriptionIdPath, 400);
    PCHECK_URI(idP->value.s, true, 0, NULL, SubscriptionIdPath, 400);

    if (subscriptionId != NULL)
    {
      if (strcmp(idP->value.s, subscriptionId) != 0)
      {
        orionldError(OrionldBadRequestData, "The Subscription ID cannot be modified", "id", 400);
        return false;
      }
    }
  }

  if (typeP != NULL)
  {
    PCHECK_STRING(typeP, 0, NULL, SubscriptionTypePath, 400);
    if (strcmp(typeP->value.s, "Subscription") != 0)
    {
      orionldError(OrionldBadRequestData, "Invalid value for Subscription Type", typeP->value.s, 400);
      return false;
    }
  }

  //
  // First we need to know whether it is a n ormal subscription or a periodic notification subscription (timeInterval)
  //
  bool pernot = false;

  KjNode* timeIntervalNodeP = kjLookup(subP, "timeInterval");
  if (timeIntervalNodeP != NULL)
  {
    PCHECK_NUMBER(timeIntervalNodeP, 0, NULL, SubscriptionTimeIntervalPath, 400);
    PCHECK_NUMBER_GT(timeIntervalNodeP, 0, "Non-supported timeInterval (must be greater than zero)", SubscriptionTimeIntervalPath, 400, 0);

    pernot = true;
    *timeInterval = (timeIntervalNodeP->type == KjInt)? timeIntervalNodeP->value.i : timeIntervalNodeP->value.f;
  }
  else
    *timeInterval = 0;


  //
  // Now loop over the entiry payload
  //
  KjNode* subItemP = subP->value.firstChildP;
  KjNode* next;
  while (subItemP != NULL)
  {
    next = subItemP->next;

    if ((strcmp(subItemP->name, "subscriptionName") == 0) || (strcmp(subItemP->name, "name") == 0))
    {
      subItemP->name = (char*) "name";  // Must be called "name" in the database
      PCHECK_STRING(subItemP, 0, NULL, SubscriptionNamePath, 400);
      PCHECK_DUPLICATE(nameP, subItemP, 0, NULL, SubscriptionNamePath, 400);
    }
    else if (strcmp(subItemP->name, "description") == 0)
    {
      PCHECK_STRING(subItemP, 0, NULL, SubscriptionDescriptionPath, 400);
      PCHECK_DUPLICATE(descriptionP,  subItemP, 0, NULL, SubscriptionDescriptionPath, 400);
    }
    else if (strcmp(subItemP->name, "entities") == 0)
    {
      PCHECK_ARRAY(subItemP, 0, NULL, SubscriptionEntitiesPath, 400);
      PCHECK_ARRAY_EMPTY(subItemP, 0, NULL, SubscriptionEntitiesPath, 400);
      PCHECK_DUPLICATE(entitiesP,  subItemP, 0, NULL, SubscriptionEntitiesPath, 400);

      if (pcheckEntityInfoArray(entitiesP, true, false, SubscriptionEntitiesPathV) == false)
        return false;
    }
    else if (strcmp(subItemP->name, "watchedAttributes") == 0)
    {
      PCHECK_DUPLICATE(watchedAttributesP, subItemP, 0, NULL, SubscriptionWatchedAttributesPath, 400);
      PCHECK_ARRAY(watchedAttributesP, 0, NULL, SubscriptionWatchedAttributesPath, 400);
      PCHECK_ARRAY_EMPTY(watchedAttributesP, 0, NULL, SubscriptionWatchedAttributesPath, 400);

      // pCheckStringArray expands (w/ orionldState.contextP) as well as checks validity
      if (pCheckStringArray(watchedAttributesP, SubscriptionWatchedAttributesItemPath, true) == false)
        return false;
    }
    else if (strcmp(subItemP->name, "timeInterval") == 0)
      PCHECK_DUPLICATE(timeIntervalP, subItemP, 0, NULL, SubscriptionTimeIntervalPath, 400);
    else if (strcmp(subItemP->name, "q") == 0)
    {
      PCHECK_DUPLICATE(qP, subItemP, 0, NULL, SubscriptionQPath, 400);
      PCHECK_STRING(qP, 0, NULL, SubscriptionQPath, 400);

      *qTreeP = qBuild(qP->value.s, qRenderedForDbP, qValidForV2P, qIsMqP, true, pernot);  // 5th parameter: qToDbModel == true
      *qNodeP = qP;

      if (*qTreeP == NULL)
        LM_RE(false, ("qBuild failed"));
    }
    else if (strcmp(subItemP->name, "geoQ") == 0)
    {
      if (*timeInterval > 0)
      {
        orionldError(OrionldOperationNotSupported, "Not Implemented", "Geo-Query is not yet implemented for Periodic Notification Subscriptions", 501);
        return false;
      }

      PCHECK_OBJECT(subItemP, 0, NULL, SubscriptionGeoqPath, 400);
      PCHECK_DUPLICATE(geoqP, subItemP, 0, NULL, SubscriptionGeoqPath, 400);

      OrionldGeoInfo* geoInfoP;
      if ((geoInfoP = pcheckGeoQ(&orionldState.kalloc, geoqP, true)) == NULL)
        return false;

      *geoCoordinatesPP = geoInfoP->coordinates;
    }
    else if (strcmp(subItemP->name, "csf") == 0)
    {
      orionldError(OrionldOperationNotSupported, "Not Implemented", "CSF (Context Source Filter) for Subscriptions", 501);
      return false;
    }
    else if (strcmp(subItemP->name, "isActive") == 0)
    {
      PCHECK_DUPLICATE(isActiveP, subItemP, 0, NULL, SubscriptionIsActivePath, 400);
      PCHECK_BOOL(isActiveP, 0, NULL, SubscriptionIsActivePath, 400);
    }
    else if (strcmp(subItemP->name, "notification") == 0)
    {
      PCHECK_OBJECT(subItemP, 0, NULL, SubscriptionNotificationPath, 400);
      PCHECK_DUPLICATE(notificationP,  subItemP, 0, NULL, SubscriptionNotificationPath, 400);
      if (pCheckNotification(notificationP, isCreate == false, uriPP, notifierInfoPP, mqttChangeP, showChangesP, sysAttrsP, renderFormatP) == false)
        return false;
    }
    else if ((strcmp(subItemP->name, "expiresAt") == 0) || (strcmp(subItemP->name, "expires") == 0))
    {
      PCHECK_STRING(subItemP, 0, NULL, SubscriptionExpiresAtPath, 400);
      PCHECK_STRING_EMPTY(subItemP, 0, NULL, SubscriptionExpiresAtPath, 400);
      PCHECK_DUPLICATE(expiresAtP, subItemP, 0, NULL, SubscriptionExpiresAtPath, 400);
      PCHECK_ISO8601(expiresAt, expiresAtP->value.s, 0, NULL, SubscriptionExpiresAtPath, 400);
      PCHECK_EXPIRESAT_IN_FUTURE(0, "Invalid Subscription", "/expiresAt/ in the past", 400, expiresAt, orionldState.requestTime);
    }
    else if (strcmp(subItemP->name, "throttling") == 0)
    {
      PCHECK_DUPLICATE(throttlingP, subItemP, 0, NULL, SubscriptionThrottlingPath, 400);
      PCHECK_NUMBER(throttlingP, 0, NULL, SubscriptionThrottlingPath, 400);
      PCHECK_NUMBER_GT(throttlingP, 0, "Negative Number not allowed in this position", SubscriptionThrottlingPath, 400, 0);
    }
    else if (strcmp(subItemP->name, "lang") == 0)
    {
      PCHECK_STRING(subItemP, 0, NULL, SubscriptionLangPath, 400);
      PCHECK_DUPLICATE(langP, subItemP, 0, NULL, SubscriptionLangPath, 400);
    }
    else if (strcmp(subItemP->name, "temporalQ") == 0)
    {
      orionldError(OrionldOperationNotSupported, "Not Implemented", SubscriptionTemporalQPath, 501);
      return false;
    }
    else if (strcmp(subItemP->name, "scopeQ") == 0)
    {
      orionldError(OrionldOperationNotSupported, "Not Implemented", SubscriptionScopePath, 501);
      return false;
    }
    else if (strcmp(subItemP->name, "status")              == 0) { kjChildRemove(subP, subItemP); }  // Silently REMOVED
    else if (strcmp(subItemP->name, "createdAt")           == 0) { kjChildRemove(subP, subItemP); }  // Silently REMOVED
    else if (strcmp(subItemP->name, "modifiedAt")          == 0) { kjChildRemove(subP, subItemP); }  // Silently REMOVED
    else if (strcmp(subItemP->name, "notificationTrigger") == 0)
    {
      orionldError(OrionldOperationNotSupported, "Not Implemented", subItemP->name, 501);
      return false;
    }
    else
    {
      orionldError(OrionldBadRequestData, "Unknown field for subscription", subItemP->name, 400);
      return false;
    }

    subItemP = next;
  }

  // Make sure all mandatory fields are present
  if (isCreate == true)
  {
    if (typeP == NULL)
    {
      orionldError(OrionldBadRequestData, "Mandatory field missing", SubscriptionTypePath, 400);
      return false;
    }
    else if (notificationP == NULL)
    {
      orionldError(OrionldBadRequestData, "Mandatory field missing", SubscriptionNotificationPath, 400);
      return false;
    }

    if (*timeInterval != 0)
    {
      if (entitiesP == NULL)
      {
        orionldError(OrionldBadRequestData, "Mandatory field missing", "'entities' is mandatory for timeInterval subscriptions" , 400);
        return false;
      }

      // Make sure it is consistent
      if (watchedAttributesP != NULL)
      {
        orionldError(OrionldBadRequestData, "Inconsistent subscription", "Both 'timeInterval' and 'watchedAttributes' are present", 400);
        return false;
      }

      if (throttlingP != NULL)
      {
        orionldError(OrionldBadRequestData, "Inconsistent subscription", "Both 'timeInterval' and 'throttling' are present", 400);
        return false;
      }
    }
    else
    {
      if ((entitiesP == NULL) && (watchedAttributesP == NULL))
      {
        orionldError(OrionldBadRequestData, "Mandatory field missing", "At least one of 'entities' and 'watchedAttributes' must be present" , 400);
        return false;
      }
    }
  }

  return true;
}
