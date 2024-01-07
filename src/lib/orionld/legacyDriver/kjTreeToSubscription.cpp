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
#include <string>                                                 // std::string

extern "C"
{
#include "kjson/KjNode.h"                                         // KjNode
#include "kjson/kjLookup.h"                                       // kjLookup
}

#include "apiTypesV2/Subscription.h"                              // Subscription

#include "orionld/common/orionldState.h"                          // orionldState
#include "orionld/common/orionldError.h"                          // orionldError
#include "orionld/common/CHECK.h"                                 // CHECKx()
#include "orionld/common/SCOMPARE.h"                              // SCOMPAREx
#include "orionld/common/uuidGenerate.h"                          // uuidGenerate
#include "orionld/payloadCheck/PCHECK.h"                          // PCHECK_URI, PCHECK_EXPIRESAT_IN_FUTURE
#include "orionld/payloadCheck/fieldPaths.h"                      // Paths to fields in the payload
#include "orionld/q/qRender.h"                                    // qRender
#include "orionld/legacyDriver/kjTreeToStringList.h"              // kjTreeToStringList
#include "orionld/legacyDriver/kjTreeToNotification.h"            // kjTreeToNotification
#include "orionld/legacyDriver/kjTreeToEntIdVector.h"             // kjTreeToEntIdVector
#include "orionld/legacyDriver/kjTreeToSubscriptionExpression.h"  // kjTreeToSubscriptionExpression
#include "orionld/legacyDriver/kjTreeToSubscription.h"            // Own interface



// -----------------------------------------------------------------------------
//
// oldTreatmentForQ -
//
bool oldTreatmentForQ(ngsiv2::Subscription* subP, char* q)
{
  bool  qWithOr = false;
  char* qOrig   = q;

  if (strchr(q, '|') != NULL)
  {
    //
    // This is a difficult situation ...
    // I need the subscription for operations that support NGSI-LD Subscription notifications.
    // Right now (April 15 2022 - with -experimental set):
    //   POST  /entities
    //   PUT   /entities/{entityId}
    //   PATCH /entities/{entityId}
    //
    // But, the Q-with-OR isn't valid for NGSiv2, so, it needs to be made unavailable for other operations.
    // Those "other operations" (the ones still using mongoBackend) use a Scope for the Q-filter, while the
    // new operations uses qLex/qParse.
    // So, the solution is to give a fake Q-filter to the Scope  (without '|') and the correct Q-filter to qLex.
    // That is fixed right here, by setting q to "P;!P" (P Exists AND P Does Not Exist) - will never match.
    //
    qWithOr = true;
    q       = (char*) "P;!P";
  }
  else if (strstr(q, "~=") != NULL)
  {
    LM_W(("Pattern Match for subscriptions - not implemented"));
    orionldError(OrionldOperationNotSupported, "Not Implemented", "Pattern matching in Q-filter", 501);
    return false;
  }

  Scope*       scopeP = new Scope(SCOPE_TYPE_SIMPLE_QUERY, qOrig);
  std::string  errorString;

  scopeP->stringFilterP = new StringFilter(SftQ);

  subP->subject.condition.expression.q = qOrig;

  if (scopeP->stringFilterP->parse(q, &errorString) == false)
  {
    LM_E(("Error parsing '%s': %s", scopeP->value.c_str(), errorString.c_str()));
    delete scopeP->stringFilterP;
    delete scopeP;
    orionldError(OrionldBadRequestData, "Invalid value for Subscription::q", errorString.c_str(), 400);
    return false;
  }

  char stringFilterExpanded[512];
  bool b;

  b = scopeP->stringFilterP->render(stringFilterExpanded, sizeof(stringFilterExpanded), &errorString);
  if (b == false)
  {
    delete scopeP->stringFilterP;
    delete scopeP;

    orionldError(OrionldInternalError, "Internal Error", "Unable to render StringFilter", 500);
    return false;
  }

  subP->subject.condition.expression.q = stringFilterExpanded;

  if (qWithOr == true)
    subP->subject.condition.expression.q = qOrig;

  subP->restriction.scopeVector.push_back(scopeP);

  return true;
}



// -----------------------------------------------------------------------------
//
// kjTreeToSubscription -
//
bool kjTreeToSubscription(ngsiv2::Subscription* subP, char** subIdPP, KjNode** endpointPP)
{
  KjNode*                   kNodeP;
  char*                     nameP                     = NULL;
  char*                     descriptionP              = NULL;
  KjNode*                   timeIntervalP             = NULL;
  KjNode*                   entitiesP                 = NULL;
  bool                      entitiesPresent           = false;
  KjNode*                   watchedAttributesP        = NULL;
  bool                      watchedAttributesPresent  = false;
  char*                     qP                        = NULL;
  KjNode*                   geoQP                     = NULL;
  bool                      geoQPresent               = false;
  char*                     csfP                      = NULL;
  bool                      isActive                  = true;
  bool                      isActivePresent           = false;
  KjNode*                   notificationP             = NULL;
  bool                      notificationPresent       = false;
  char*                     expiresP                  = NULL;
  bool                      throttlingPresent         = false;

  //
  // Default values
  //
  subP->attrsFormat                        = RF_NORMALIZED;
  subP->descriptionProvided                = false;
  subP->expires                            = 0;
  subP->throttling                         = 0;
  subP->subject.condition.expression.isSet = false;
  subP->notification.blacklist             = false;
  subP->notification.timesSent             = 0;
  subP->notification.lastNotification      = 0;
  subP->notification.lastFailure           = 0;
  subP->notification.lastSuccess           = 0;
  subP->notification.httpInfo.verb         = HTTP_POST;
  subP->notification.httpInfo.custom       = false;
  subP->notification.httpInfo.mimeType     = MT_JSON;


  //
  // Already pretreated (by orionMhdConnectionTreat):
  //
  // o @context (if any)
  // o id
  // o type
  //
  if (orionldState.payloadIdNode == NULL)
  {
    char subscriptionId[80];
    uuidGenerate(subscriptionId, sizeof(subscriptionId), "urn:ngsi-ld:Subscription:");
    subP->id = subscriptionId;
  }
  else
  {
    char* uri = orionldState.payloadIdNode->value.s;
    PCHECK_URI(uri, true, 0, "Subscription::id is not a URI", uri, 400);
    subP->id = uri;
  }

  *subIdPP  = (char*) subP->id.c_str();


  //
  // type
  //
  // NOTE
  //   The spec of ngsi-ld states that the field "type" is MANDATORY and MUST be set to "Subscription".
  //   A bit funny in my opinion.
  //   However, here we make sure that the spec is followed, but we add nothing to the database.
  //   When rendering (serializing) subscriptions for GET /subscriptions, the field
  //     "type": "Subscription"
  //   is added to the response payload.
  //
  if (orionldState.payloadTypeNode == NULL)
  {
    orionldError(OrionldBadRequestData, "Mandatory field missing", "Subscription::type", 400);
    return false;
  }

  if (!SCOMPARE13(orionldState.payloadTypeNode->value.s, 'S', 'u', 'b', 's', 'c', 'r', 'i', 'p', 't', 'i', 'o', 'n', 0))
  {
    orionldError(OrionldBadRequestData, "Invalid value for Subscription::type", orionldState.payloadTypeNode->value.s, 400);
    return false;
  }


  //
  // Loop over the tree
  //
  for (kNodeP = orionldState.requestTree->value.firstChildP; kNodeP != NULL; kNodeP = kNodeP->next)
  {
    if ((strcmp(kNodeP->name, "name") == 0) || (strcmp(kNodeP->name, "subscriptionName") == 0))
    {
      DUPLICATE_CHECK(nameP, "Subscription::name", kNodeP->value.s);
      STRING_CHECK(kNodeP, "Subscription::name");
      subP->name = nameP;
    }
    else if (strcmp(kNodeP->name, "description") == 0)
    {
      DUPLICATE_CHECK(descriptionP, "Subscription::description", kNodeP->value.s);
      STRING_CHECK(kNodeP, "Subscription::description");

      subP->description         = descriptionP;
      subP->descriptionProvided = true;
    }
    else if (strcmp(kNodeP->name, "entities") == 0)
    {
      DUPLICATE_CHECK_WITH_PRESENCE(entitiesPresent, entitiesP, SubscriptionEntitiesPath, kNodeP);
      ARRAY_CHECK(kNodeP, SubscriptionEntitiesPath);
      EMPTY_ARRAY_CHECK(kNodeP, SubscriptionEntitiesPath);

      if (kjTreeToEntIdVector(entitiesP, &subP->subject.entities) == false)
      {
        LM_E(("kjTreeToEntIdVector failed"));
        return false;  // orionldError is invoked by kjTreeToEntIdVector
      }
    }
    else if (strcmp(kNodeP->name, "watchedAttributes") == 0)
    {
      DUPLICATE_CHECK_WITH_PRESENCE(watchedAttributesPresent, watchedAttributesP, "Subscription::watchedAttributes", kNodeP);
      ARRAY_CHECK(kNodeP, "Subscription::watchedAttributes");
      EMPTY_ARRAY_CHECK(kNodeP, "Subscription::watchedAttributes");

      if (kjTreeToStringList(watchedAttributesP, &subP->subject.condition.attributes) == false)
      {
        LM_E(("kjTreeToStringList failed"));
        return false;  // orionldError is invoked by kjTreeToStringList
      }
    }
    else if (strcmp(kNodeP->name, "timeInterval") == 0)
    {
      orionldError(OrionldOperationNotSupported, "Not Implemented", "Periodic Notification Subscriptions are not implemented", 501);
      return false;
    }
    else if ((kNodeP->name[0] == 'q') && (kNodeP->name[1] == 0))
    {
      STRING_CHECK(kNodeP, "Subscription::q");
      DUPLICATE_CHECK(qP, "Subscription::q", kNodeP->value.s);

      if (oldTreatmentForQ(subP, kNodeP->value.s) == false)  // New treatment doesn't even use this function - just the Kj Tree
        return false;
    }
    else if (strcmp(kNodeP->name, "geoQ") == 0)
    {
      DUPLICATE_CHECK_WITH_PRESENCE(geoQPresent, geoQP, "Subscription::geoQ", kNodeP);
      OBJECT_CHECK(kNodeP, "Subscription::geoQ");

      if (kjTreeToSubscriptionExpression(geoQP, &subP->subject.condition.expression) == false)
      {
        LM_E(("kjTreeToSubscriptionExpression failed"));
        return false;  // orionldError is invoked by kjTreeToSubscriptionExpression
      }
    }
    else if (strcmp(kNodeP->name, "csf") == 0)
    {
      DUPLICATE_CHECK(csfP, "Subscription::csf", kNodeP->value.s);
      STRING_CHECK(kNodeP, "Subscription::csf");
      subP->csf = csfP;
    }
    else if (strcmp(kNodeP->name, "isActive") == 0)
    {
      DUPLICATE_CHECK_WITH_PRESENCE(isActivePresent, isActive, "Subscription::isActive", kNodeP->value.b);
      BOOL_CHECK(kNodeP, "Subscription::isActive");
      subP->status = (isActive == true)? "active" : "paused";
    }
    else if (strcmp(kNodeP->name, "notification") == 0)
    {
      DUPLICATE_CHECK_WITH_PRESENCE(notificationPresent, notificationP, "Subscription::notification", kNodeP);
      OBJECT_CHECK(kNodeP, "Subscription::notification");

      if (kjTreeToNotification(notificationP, subP, endpointPP) == false)
      {
        LM_E(("kjTreeToNotification failed"));
        return false;  // orionldError is invoked by kjTreeToNotification
      }
    }
    else if ((strcmp(kNodeP->name, "expires") == 0) || (strcmp(kNodeP->name, "expiresAt") == 0))
    {
      DUPLICATE_CHECK(expiresP, "Subscription::expiresAt", kNodeP->value.s);
      STRING_CHECK(kNodeP, "Subscription::expiresAt");
      DATETIME_CHECK(expiresP, subP->expires, "Subscription::expiresAt");
      PCHECK_EXPIRESAT_IN_FUTURE(0, "Invalid Subscription", "/expiresAt/ in the past", 400, subP->expires, orionldState.requestTime);
    }
    else if (strcmp(kNodeP->name, "throttling") == 0)
    {
      if (throttlingPresent == true)
      {
        orionldError(OrionldBadRequestData, "Duplicated field", kNodeP->name, 400);
        return false;
      }
      throttlingPresent = true;

      if (kNodeP->type == KjFloat)
        subP->throttling = kNodeP->value.f;
      else if (kNodeP->type == KjInt)
        subP->throttling = kNodeP->value.i;
      else
      {
        orionldError(OrionldBadRequestData, "Not a JSON Number", "Subscription::throttling", 400);
        return false;
      }
    }
    else if (strcmp(kNodeP->name, "lang") == 0)
    {
      subP->lang = kNodeP->value.s;
    }
    else if (strcmp(kNodeP->name, "status") == 0)
    {
      // Ignored - read-only
    }
    else if (strcmp(kNodeP->name, "createdAt") == 0)
    {
      // Ignored - read-only
    }
    else if (strcmp(kNodeP->name, "modifiedAt") == 0)
    {
      // Ignored - read-only
    }
    else
    {
      orionldError(OrionldBadRequestData, "Invalid field for Subscription", kNodeP->name, 400);
      return false;
    }
  }

  if ((entitiesPresent == false) && (watchedAttributesPresent == false))
  {
    orionldError(OrionldBadRequestData, "At least one of 'entities' and 'watchedAttributes' must be present", NULL, 400);
    return false;
  }

  if ((timeIntervalP != NULL) && (watchedAttributesPresent == true))
  {
    orionldError(OrionldBadRequestData, "Inconsistent subscription", "Both 'timeInterval' and 'watchedAttributes' present", 400);
    return false;
  }

  if (notificationP == NULL)
  {
    orionldError(OrionldBadRequestData, "Mandatory field missing", "Subscription::notification", 400);
    return false;
  }

  return true;
}
