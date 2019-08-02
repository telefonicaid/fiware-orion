/*
*
* Copyright 2019 Telefonica Investigacion y Desarrollo, S.A.U
*
* This file is part of Orion Context Broker.
*
* Orion Context Broker is free software: you can redistribute it and/or
* modify it under the terms of the GNU Affero General Public License as
* published by the Free Software Foundation, either version 3 of the
* License, or (at your option) any later version.
*
* Orion Context Broker is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero
* General Public License for more details.
*
* You should have received a copy of the GNU Affero General Public License
* along with Orion Context Broker. If not, see http://www.gnu.org/licenses/.
*
* For those usages not covered by this license please contact with
* iot_support at tid dot es
*
* Author: Ken Zangelin
*/
#include <string>                                              // std::string

extern "C"
{
#include "kjson/KjNode.h"                                      // KjNode
}

#include "rest/ConnectionInfo.h"                               // ConnectionInfo
#include "apiTypesV2/Subscription.h"                           // Subscription
#include "mongoBackend/MongoGlobal.h"                          // mongoIdentifier

#include "orionld/common/CHECK.h"                              // CHECKx()
#include "orionld/common/SCOMPARE.h"                           // SCOMPAREx
#include "orionld/common/orionldState.h"                       // orionldState
#include "orionld/common/orionldErrorResponse.h"               // orionldErrorResponseCreate
#include "orionld/common/urlCheck.h"                           // urlCheck
#include "orionld/common/urnCheck.h"                           // urnCheck
#include "orionld/context/orionldContextTreat.h"               // orionldContextTreat
#include "orionld/kjTree/kjTreeToEntIdVector.h"                // kjTreeToEntIdVector
#include "orionld/kjTree/kjTreeToStringList.h"                 // kjTreeToStringList
#include "orionld/kjTree/kjTreeToSubscriptionExpression.h"     // kjTreeToSubscriptionExpression
#include "orionld/kjTree/kjTreeToNotification.h"               // kjTreeToNotification
#include "orionld/kjTree/kjTreeToSubscription.h"               // Own interface



// -----------------------------------------------------------------------------
//
// kjTreeToSubscription -
//
bool kjTreeToSubscription(ConnectionInfo* ciP, ngsiv2::Subscription* subP, char** subIdPP)
{
  KjNode*                   kNodeP;
  char*                     nameP                     = NULL;
  char*                     descriptionP              = NULL;
  char*                     timeIntervalP             = NULL;
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
  uint64_t                  throttling                = -1;
  bool                      throttlingPresent         = false;

  //
  // Default values
  //
  subP->attrsFormat                        = NGSI_LD_V1_KEYVALUES;
  subP->descriptionProvided                = false;
  subP->expires                            = 0x7FFFFFFF;
  subP->throttling                         = 0;
  subP->subject.condition.expression.isSet = false;
  subP->notification.blacklist             = false;
  subP->notification.timesSent             = 0;
  subP->notification.lastNotification      = 0;
  subP->notification.lastFailure           = 0;
  subP->notification.lastSuccess           = 0;
  subP->notification.httpInfo.verb         = POST;
  subP->notification.httpInfo.custom       = false;
  subP->notification.httpInfo.mimeType     = JSON;


  //
  // Already pretreated (by orionMhdConnectionTreat):
  //
  // o @context (if any)
  // o id
  // o type
  //

  if (orionldState.payloadIdNode == NULL)
  {
    char randomId[32];

    mongoIdentifier(randomId);

    subP->id  = "urn:ngsi-ld:Subscription:";
    subP->id += randomId;
  }
  else
    subP->id = orionldState.payloadIdNode->value.s;

  if ((urlCheck((char*) subP->id.c_str(), NULL) == false) && (urnCheck((char*) subP->id.c_str(), NULL) == false))
  {
    LM_W(("Bad Input (Subscription::id is not a URI)"));
    orionldErrorResponseCreate(ciP, OrionldBadRequestData, "Subscription::id is not a URI", subP->id.c_str(), OrionldDetailsAttribute);
    ciP->httpStatusCode = SccBadRequest;
    return false;
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
    LM_W(("Bad Input (Mandatory field missing: Subscription::type)"));
    orionldErrorResponseCreate(ciP, OrionldBadRequestData, "Mandatory field missing", "Subscription::type", OrionldDetailsString);
    ciP->httpStatusCode = SccBadRequest;
    return false;
  }

  if (!SCOMPARE13(orionldState.payloadTypeNode->value.s, 'S', 'u', 'b', 's', 'c', 'r', 'i', 'p', 't', 'i', 'o', 'n', 0))
  {
    LM_W(("Bad Input (subscription type must have the value /Subscription/)"));
    orionldErrorResponseCreate(ciP, OrionldBadRequestData, "Invalid value for Subscription::type", orionldState.payloadTypeNode->value.s, OrionldDetailsString);
    ciP->httpStatusCode = SccBadRequest;
    return false;
  }


  //
  // Loop over the tree
  //
  for (kNodeP = orionldState.requestTree->value.firstChildP; kNodeP != NULL; kNodeP = kNodeP->next)
  {
    if (SCOMPARE5(kNodeP->name, 'n', 'a', 'm', 'e', 0))
    {
      DUPLICATE_CHECK(nameP, "Subscription::name", kNodeP->value.s);
      STRING_CHECK(kNodeP, "Subscription::name");
      subP->name = nameP;
    }
    else if (SCOMPARE12(kNodeP->name, 'd', 'e', 's', 'c', 'r', 'i', 'p', 't', 'i', 'o', 'n', 0))
    {
      DUPLICATE_CHECK(descriptionP, "Subscription::description", kNodeP->value.s);
      STRING_CHECK(kNodeP, "Subscription::description");

      subP->description         = descriptionP;
      subP->descriptionProvided = true;
    }
    else if (SCOMPARE9(kNodeP->name, 'e', 'n', 't', 'i', 't', 'i', 'e', 's', 0))
    {
      DUPLICATE_CHECK_WITH_PRESENCE(entitiesPresent, entitiesP, "Subscription::entities", kNodeP);
      ARRAY_CHECK(kNodeP, "Subscription::entities");
      EMPTY_ARRAY_CHECK(kNodeP, "Subscription::entities");

      if (kjTreeToEntIdVector(ciP, entitiesP, &subP->subject.entities) == false)
      {
        LM_E(("kjTreeToEntIdVector failed"));
        return false;  // orionldErrorResponseCreate is invoked by kjTreeToEntIdVector
      }
    }
    else if (SCOMPARE18(kNodeP->name, 'w', 'a', 't', 'c', 'h', 'e', 'd', 'A', 't', 't', 'r', 'i', 'b', 'u', 't', 'e', 's', 0))
    {
      DUPLICATE_CHECK_WITH_PRESENCE(watchedAttributesPresent, watchedAttributesP, "Subscription::watchedAttributes", kNodeP);
      ARRAY_CHECK(kNodeP, "Subscription::watchedAttributes");
      EMPTY_ARRAY_CHECK(kNodeP, "Subscription::watchedAttributes");

      if (kjTreeToStringList(ciP, watchedAttributesP, &subP->subject.condition.attributes) == false)
      {
        LM_E(("kjTreeToStringList failed"));
        return false;  // orionldErrorResponseCreate is invoked by kjTreeToStringList
      }
    }
    else if (SCOMPARE13(kNodeP->name, 't', 'i', 'm', 'e', 'I', 'n', 't', 'e', 'r', 'v', 'a', 'l', 0))
    {
      DUPLICATE_CHECK(timeIntervalP, "Subscription::timeInterval", kNodeP->value.s);
      INTEGER_CHECK(kNodeP, "Subscription::timeInterval");
      // FIXME: Implement this ???
    }
    else if (SCOMPARE2(kNodeP->name, 'q', 0))
    {
      DUPLICATE_CHECK(qP, "Subscription::q", kNodeP->value.s);
      STRING_CHECK(kNodeP, "Subscription::q");

      Scope*       scopeP = new Scope(SCOPE_TYPE_SIMPLE_QUERY, kNodeP->value.s);
      std::string  errorString;

      scopeP->stringFilterP = new StringFilter(SftQ);

      if (scopeP->stringFilterP->parse(scopeP->value.c_str(), &errorString) == false)
      {
        delete scopeP->stringFilterP;
        delete scopeP;

        orionldErrorResponseCreate(ciP, OrionldBadRequestData, "Invalid value for Subscription::q", kNodeP->value.s, OrionldDetailsString);
        return false;
      }

      char stringFilterExpanded[512];
      bool b;

      b = scopeP->stringFilterP->render(stringFilterExpanded, sizeof(stringFilterExpanded), &errorString);
      if (b == false)
      {
        delete scopeP->stringFilterP;
        delete scopeP;

        orionldErrorResponseCreate(ciP, OrionldInternalError, "Internal Error", "Unable to render StringFilter", OrionldDetailsString);
        return false;
      }

      subP->subject.condition.expression.q = stringFilterExpanded;
      subP->restriction.scopeVector.push_back(scopeP);
    }
    else if (SCOMPARE5(kNodeP->name, 'g', 'e', 'o', 'Q', 0))
    {
      DUPLICATE_CHECK_WITH_PRESENCE(geoQPresent, geoQP, "Subscription::geoQ", kNodeP);
      OBJECT_CHECK(kNodeP, "Subscription::geoQ");

      if (kjTreeToSubscriptionExpression(ciP, geoQP, &subP->subject.condition.expression) == false)
      {
        LM_E(("kjTreeToSubscriptionExpression failed"));
        return false;  // orionldErrorResponseCreate is invoked by kjTreeToSubscriptionExpression
      }
    }
    else if (SCOMPARE4(kNodeP->name, 'c', 's', 'f', 0))
    {
      DUPLICATE_CHECK(csfP, "Subscription::csf", kNodeP->value.s);
      STRING_CHECK(kNodeP, "Subscription::csf");
      LM_W(("ToDo: Implement Subscription::csf"));
    }
    else if (SCOMPARE9(kNodeP->name, 'i', 's', 'A', 'c', 't', 'i', 'v', 'e', 0))
    {
      DUPLICATE_CHECK_WITH_PRESENCE(isActivePresent, isActive, "Subscription::isActive", kNodeP->value.b);
      BOOL_CHECK(kNodeP, "Subscription::isActive");
      subP->status = (isActive == true)? "active" : "paused";
    }
    else if (SCOMPARE13(kNodeP->name, 'n', 'o', 't', 'i', 'f', 'i', 'c', 'a', 't', 'i', 'o', 'n', 0))
    {
      DUPLICATE_CHECK_WITH_PRESENCE(notificationPresent, notificationP, "Subscription::notification", kNodeP);
      OBJECT_CHECK(kNodeP, "Subscription::notification");

      if (kjTreeToNotification(ciP, notificationP, subP) == false)
      {
        LM_E(("kjTreeToNotification failed"));
        return false;  // orionldErrorResponseCreate is invoked by kjTreeToNotification
      }
    }
    else if (SCOMPARE8(kNodeP->name, 'e', 'x', 'p', 'i', 'r', 'e', 's', 0))
    {
      DUPLICATE_CHECK(expiresP, "Subscription::expires", kNodeP->value.s);
      STRING_CHECK(kNodeP, "Subscription::expires");
      DATETIME_CHECK(expiresP, "Subscription::expires");

      subP->expires = parse8601Time(expiresP);
    }
    else if (SCOMPARE11(kNodeP->name, 't', 'h', 'r', 'o', 't', 't', 'l', 'i', 'n', 'g', 0))
    {
      DUPLICATE_CHECK_WITH_PRESENCE(throttlingPresent, throttling, "Subscription::throttling", kNodeP->value.i);
      INTEGER_CHECK(kNodeP, "Subscription::throttling");

      subP->throttling = throttling;
    }
    else if (SCOMPARE7(kNodeP->name, 's', 't', 'a', 't', 'u', 's', 0))
    {
      // Ignored - read-only
    }
    else if (SCOMPARE10(kNodeP->name, 'c', 'r', 'e', 'a', 't', 'e', 'd', 'A', 't', 0))
    {
      // Ignored - read-only
    }
    else if (SCOMPARE11(kNodeP->name, 'm', 'o', 'd', 'i', 'f', 'i', 'e', 'd', 'A', 't', 0))
    {
      // Ignored - read-only
    }
    else
    {
      orionldErrorResponseCreate(ciP, OrionldBadRequestData, "Invalid field for Subscription", kNodeP->name, OrionldDetailsString);
      return false;
    }
  }

  if ((entitiesPresent == false) && (watchedAttributesPresent == false))
  {
    LM_E(("At least one of 'entities' and 'watchedAttributes' must be present"));
    orionldErrorResponseCreate(ciP, OrionldBadRequestData, "At least one of 'entities' and 'watchedAttributes' must be present", NULL, OrionldDetailsString);
    return false;
  }

  if ((timeIntervalP != NULL) && (watchedAttributesPresent == true))
  {
    LM_E(("Both 'timeInterval' and 'watchedAttributes' present"));
    orionldErrorResponseCreate(ciP, OrionldBadRequestData, "Both 'timeInterval' and 'watchedAttributes' present", NULL, OrionldDetailsString);
    return false;
  }

  if (notificationP == NULL)
  {
    LM_E(("Notification Parameters missing in Subscription"));
    orionldErrorResponseCreate(ciP, OrionldBadRequestData, "Mandatory Field Missing", "Subscription::notification", OrionldDetailsString);
    return false;
  }

  return true;
}
