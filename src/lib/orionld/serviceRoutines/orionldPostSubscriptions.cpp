/*
*
* Copyright 2018 Telefonica Investigacion y Desarrollo, S.A.U
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
#include "logMsg/logMsg.h"                                     // LM_*
#include "logMsg/traceLevels.h"                                // Lmt*

extern "C"
{
#include "kjson/kjRender.h"                                    // kjRender
}

#include "common/globals.h"                                    // parse8601Time
#include "rest/OrionError.h"                                   // OrionError
#include "rest/ConnectionInfo.h"                               // ConnectionInfo
#include "rest/httpHeaderAdd.h"                                // httpHeaderAdd, httpHeaderLinkAdd, httpHeaderLocationAdd
#include "apiTypesV2/HttpInfo.h"                               // HttpInfo
#include "apiTypesV2/Subscription.h"                           // Subscription
#include "apiTypesV2/EntID.h"                                  // EntID
#include "mongoBackend/mongoCreateSubscription.h"              // mongoCreateSubscription
#include "mongoBackend/MongoGlobal.h"                          // mongoIdentifier
#include "orionld/common/urlCheck.h"                           // urlCheck
#include "orionld/common/urnCheck.h"                           // urnCheck
#include "orionld/common/SCOMPARE.h"                           // SCOMPAREx
#include "orionld/common/CHECK.h"                              // CHECKx(U)
#include "orionld/common/orionldErrorResponse.h"               // orionldErrorResponseCreate
#include "orionld/context/orionldCoreContext.h"                // ORIONLD_CORE_CONTEXT_URL
#include "orionld/context/orionldContextTreat.h"               // orionldContextTreat
#include "orionld/context/orionldUriExpand.h"                  // orionldUriExpand
#include "orionld/serviceRoutines/orionldPostSubscriptions.h"  // Own Interface



// -----------------------------------------------------------------------------
//
// ktreeToEntities -
//
// ngsiv2::EntID is used, but the type is really EntityInfo, and it has three fields:
// - id
// - idPattern
// - type
//
static bool ktreeToEntities(ConnectionInfo* ciP, KjNode* kNodeP, std::vector<ngsiv2::EntID>* entitiesP)
{
  KjNode* entityP;

  for (entityP = kNodeP->value.firstChildP; entityP != NULL; entityP = entityP->next)
  {
    if (entityP->type != KjObject)
    {
      orionldErrorResponseCreate(ciP, OrionldBadRequestData, "EntityInfo array member not a JSON Object", NULL, OrionldDetailsString);
      return false;
    }

    KjNode*  itemP;
    char*    idP        = NULL;
    char*    idPatternP = NULL;
    char*    typeP      = NULL;

    for (itemP = entityP->value.firstChildP; itemP != NULL; itemP = itemP->next)
    {
      if (SCOMPARE3(itemP->name, 'i', 'd', 0))
      {
        DUPLICATE_CHECK(idP, "EntityInfo::id", itemP->value.s);
        STRING_CHECK(itemP, "EntityInfo::id");
      }
      else if (SCOMPARE10(itemP->name, 'i', 'd', 'P', 'a', 't', 't', 'e', 'r', 'n', 0))
      {
        DUPLICATE_CHECK(idPatternP, "EntityInfo::idPattern", itemP->value.s);
        STRING_CHECK(itemP, "EntityInfo::idPattern");
      }
      else if (SCOMPARE5(itemP->name, 't', 'y', 'p', 'e', 0))
      {
        DUPLICATE_CHECK(typeP, "EntityInfo::type", itemP->value.s);
        STRING_CHECK(itemP, "EntityInfo::type");
      }
      else
      {
        orionldErrorResponseCreate(ciP, OrionldBadRequestData, "Unknown EntityInfo field", itemP->name, OrionldDetailsString);
        return false;
      }
    }

    if ((idP == NULL) && (idPatternP == NULL) && (typeP == NULL))
    {
      orionldErrorResponseCreate(ciP, OrionldBadRequestData, "Empty EntityInfo object", NULL, OrionldDetailsString);
      return false;
    }

    if ((idP != NULL) && (idPatternP != NULL))
    {
      orionldErrorResponseCreate(ciP, OrionldBadRequestData, "Both 'id' and 'idPattern' given in EntityInfo object", NULL, OrionldDetailsString);
      return false;
    }

    if (idP != NULL)
    {
      // The entity id must be a URI
      char* details;

      if ((urlCheck(idP, &details) == false) && (urnCheck(idP, &details) == false))
      {
        orionldErrorResponseCreate(ciP, OrionldBadRequestData, "Invalid Entity ID", details, OrionldDetailsString);
        ciP->httpStatusCode = SccBadRequest;
        return false;
      }
    }

    if (typeP == NULL)
    {
      orionldErrorResponseCreate(ciP, OrionldBadRequestData, "Missing field in EntityInfo object", "type", OrionldDetailsString);
      return false;
    }

    char  typeExpanded[256];
    char* details;

    if (orionldUriExpand(ciP->contextP, typeP, typeExpanded, sizeof(typeExpanded), &details) == false)
    {
      orionldErrorResponseCreate(ciP, OrionldBadRequestData, "Error during URI expansion of entity type", details, OrionldDetailsString);
      return false;
    }
    typeP = typeExpanded;

    ngsiv2::EntID entityInfo;

    if (idP)        entityInfo.id        = idP;
    if (idPatternP) entityInfo.idPattern = idPatternP;
    if (typeP)      entityInfo.type      = typeP;

    entitiesP->push_back(entityInfo);
  }

  return true;
}



// -----------------------------------------------------------------------------
//
// ktreeToStringList -
//
static bool ktreeToStringList(ConnectionInfo* ciP, KjNode* kNodeP, std::vector<std::string>* stringListP)
{
  KjNode* attributeP;

  for (attributeP = kNodeP->value.firstChildP; attributeP != NULL; attributeP = attributeP->next)
  {
    char  expanded[256];
    char* details;

    STRING_CHECK(attributeP, "String-List item");

    if (orionldUriExpand(ciP->contextP, attributeP->value.s, expanded, sizeof(expanded), &details) == false)
    {
      orionldErrorResponseCreate(ciP, OrionldBadRequestData, "Error during URI expansion of entity type", details, OrionldDetailsString);
      delete stringListP;  // ?
      return false;
    }

    stringListP->push_back(expanded);
  }

  return true;
}



// -----------------------------------------------------------------------------
//
// ktreeToSubscriptionExpression -
//
static bool ktreeToSubscriptionExpression(ConnectionInfo* ciP, KjNode* kNodeP, SubscriptionExpression* subExpressionP)
{
  KjNode*  itemP;
  char*    geometryP          = NULL;
  KjNode*  coordinatesNodeP   = NULL;
  char*    georelP            = NULL;
  char*    geoPropertyP       = NULL;

  for (itemP = kNodeP->value.firstChildP; itemP != NULL; itemP = itemP->next)
  {
    if (SCOMPARE9(itemP->name, 'g', 'e', 'o', 'm', 'e', 't', 'r', 'y', 0))
    {
      DUPLICATE_CHECK(geometryP, "GeoQuery::geometry", itemP->value.s);
      STRING_CHECK(itemP, "GeoQuery::geometry");
    }
    else if (SCOMPARE12(itemP->name, 'c', 'o', 'o', 'r', 'd', 'i', 'n', 'a', 't', 'e', 's', 0))
    {
      if (itemP->type == KjString)
      {
        DUPLICATE_CHECK(coordinatesNodeP, "GeoQuery::coordinates", itemP);
      }
      else if (itemP->type == KjArray)
      {
        DUPLICATE_CHECK(coordinatesNodeP, "GeoQuery::coordinates", itemP);
      }
      else
      {
        orionldErrorResponseCreate(ciP, OrionldBadRequestData, "Invalid value type (not String nor Array)", "GeoQuery::coordinates", OrionldDetailsString);
        return false;
      }
      LM_T(LmtGeoJson, ("coordinatesNodeP->type: %s", kjValueType(coordinatesNodeP->type)));
    }
    else if (SCOMPARE7(itemP->name, 'g', 'e', 'o', 'r', 'e', 'l', 0))
    {
      DUPLICATE_CHECK(georelP, "GeoQuery::georel", itemP->value.s);
      STRING_CHECK(itemP, "GeoQuery::georel");
    }
    else if (SCOMPARE12(itemP->name, 'g', 'e', 'o', 'p', 'r', 'o', 'p', 'e', 'r', 't', 'y', 0))
    {
      DUPLICATE_CHECK(geoPropertyP, "GeoQuery::geoproperty", itemP->value.s);
      STRING_CHECK(itemP, "GeoQuery::geoproperty");
    }
    else
    {
      orionldErrorResponseCreate(ciP, OrionldBadRequestData, "Unknown GeoQuery field", itemP->name, OrionldDetailsString);
      return false;
    }
  }

  if (geometryP == NULL)
  {
    orionldErrorResponseCreate(ciP, OrionldBadRequestData, "GeoQuery::geometry missing in Subscription", NULL, OrionldDetailsString);
    return false;
  }

  if (coordinatesNodeP == NULL)
  {
    orionldErrorResponseCreate(ciP, OrionldBadRequestData, "GeoQuery::coordinates missing in Subscription", NULL, OrionldDetailsString);
    return false;
  }

  if (georelP == NULL)
  {
    orionldErrorResponseCreate(ciP, OrionldBadRequestData, "GeoQuery::georel missing in Subscription", NULL, OrionldDetailsString);
    return false;
  }

  LM_T(LmtGeoJson, ("coordinatesNodeP->type: %s", kjValueType(coordinatesNodeP->type)));
  if (coordinatesNodeP->type == KjArray)
  {
    char coords[512];

    //
    // We have a little problem here ...
    // SubscriptionExpression::coords is a std::string in APIv2.
    // NGSI-LD needs it to be able to be an array.
    // Easiest way to fix this is to render the JSON Array and translate it to a string, and then removing the '[]'
    //
    kjRender(ciP->kjsonP, coordinatesNodeP, coords, sizeof(coords));
    coords[strlen(coords) - 1] = 0;
    LM_T(LmtGeoJson, ("Rendered coords array: '%s'", &coords[1]));
    subExpressionP->coords = &coords[1];
  }
  else
    subExpressionP->coords = coordinatesNodeP->value.s;

  subExpressionP->geometry = geometryP;
  subExpressionP->georel   = georelP;

  //
  // FIXME: geoproperty is not part of SubscriptionExpression in APIv2.
  //        For now, we skip geoproperty and only work on 'location'.
  //

  return true;
}



// -----------------------------------------------------------------------------
//
// ktreeToEndpoint -
//
static bool ktreeToEndpoint(ConnectionInfo* ciP, KjNode* kNodeP, ngsiv2::HttpInfo* httpInfoP)
{
  char* uriP    = NULL;
  char* acceptP = NULL;
  char* details;

  // Set default values
  httpInfoP->mimeType = JSON;
  
  for (KjNode* itemP = kNodeP->value.firstChildP; itemP != NULL; itemP = itemP->next)
  {
    if (SCOMPARE4(itemP->name, 'u', 'r', 'i', 0))
    {
      DUPLICATE_CHECK(uriP, "Endpoint::uri", itemP->value.s);
      STRING_CHECK(itemP, "Endpoint::uri");

      if (!urlCheck(uriP, &details) && !urnCheck(uriP, &details))
      {
        orionldErrorResponseCreate(ciP, OrionldBadRequestData, "Invalid Endpoint::uri", "Not a URL nor a URN", OrionldDetailsString);
        return false;
      }

      httpInfoP->url = uriP;
    }
    else if (SCOMPARE7(itemP->name, 'a', 'c', 'c', 'e', 'p', 't', 0))
    {
      DUPLICATE_CHECK(acceptP, "Endpoint::accept", itemP->value.s);
      STRING_CHECK(itemP, "Endpoint::accept");
      char* mimeType = itemP->value.s;
      LM_TMP(("Accept: %s", mimeType));
      if (!SCOMPARE12(mimeType, 'a', 'p', 'p', 'l', 'i', 'c', 'a', 't', 'i', 'o', 'n', '/'))
      {
        orionldErrorResponseCreate(ciP, OrionldBadRequestData, "Invalid Endpoint::accept value", mimeType, OrionldDetailsString);
        return false;
      }

      // Already accepted the first 12 chars (application/), step over them
      mimeType = &mimeType[12];
      if (SCOMPARE5(mimeType, 'j', 's', 'o', 'n', 0))
        httpInfoP->mimeType = JSON;
      else if (SCOMPARE8(mimeType, 'l', 'd', '+', 'j', 's', 'o', 'n', 0))
        httpInfoP->mimeType = JSONLD;
      else
      {
        orionldErrorResponseCreate(ciP, OrionldBadRequestData, "Invalid Endpoint::accept value", itemP->value.s, OrionldDetailsString);
        return false;
      }
    }
    else
    {
      orionldErrorResponseCreate(ciP, OrionldBadRequestData, "Unrecognized field in Endpoint", itemP->name, OrionldDetailsString);
      return false;
    }
  }

  if (uriP == NULL)
  {
    orionldErrorResponseCreate(ciP, OrionldBadRequestData, "Mandatory field missing", "Endpoint::uri", OrionldDetailsString);
    return false;
  }

  return true;
}



// -----------------------------------------------------------------------------
//
// formatExtract -
//
static bool formatExtract(ConnectionInfo* ciP, char* format, ngsiv2::Subscription* subP)
{
  if (SCOMPARE10(format, 'k', 'e', 'y', 'V', 'a', 'l', 'u', 'e', 's', 0))
    subP->attrsFormat = NGSI_LD_V1_KEYVALUES;
  else if (SCOMPARE11(format, 'n', 'o', 'r', 'm', 'a', 'l', 'i', 'z', 'e', 'd', 0))
    subP->attrsFormat = NGSI_LD_V1_NORMALIZED;
  else
  {
    LM_E(("Invalid value for Notification::format: '%s'", format));
    orionldErrorResponseCreate(ciP, OrionldBadRequestData, "Invalid value for Notification::format", format, OrionldDetailsString);
    return false;
  }

  return true;
}



// -----------------------------------------------------------------------------
//
// ktreeToNotification -
//
// 0..1  attributes         Array of Strings
// 0..1  format             String ("keyValues" or "normalized")
//    1  endpoint           Object?
// 0..1  status             String: "ok" or "failed" - internal !
//
static bool ktreeToNotification(ConnectionInfo* ciP, KjNode* kNodeP, ngsiv2::Subscription* subP)
{
  KjNode*   attributesP   = NULL;
  char*     formatP       = NULL;
  KjNode*   endpointP     = NULL;
  KjNode*   itemP;

  // Set default values
  subP->attrsFormat = NGSI_LD_V1_NORMALIZED; 

  // Extract the info from the kjTree
  for (itemP = kNodeP->value.firstChildP; itemP != NULL; itemP = itemP->next)
  {
    if (SCOMPARE11(itemP->name, 'a', 't', 't', 'r', 'i', 'b', 'u', 't', 'e', 's', 0))
    {
      DUPLICATE_CHECK(attributesP, "Notification::attributes", itemP);
      ARRAY_CHECK(itemP, "Notification::attributes");

      if (ktreeToStringList(ciP, itemP, &subP->notification.attributes) == false)
        return false;
    }
    else if (SCOMPARE7(itemP->name, 'f', 'o', 'r', 'm', 'a', 't', 0))
    {
      DUPLICATE_CHECK(formatP, "Notification::format", itemP->value.s);
      STRING_CHECK(itemP, "Notification::format");

      LM_T(LmtNotificationFormat, ("Got a subscription format: '%s'", itemP->value.s));
      if (formatExtract(ciP, formatP, subP) == false)
        return false;
      LM_T(LmtNotificationFormat, ("Extracted subscription format: %d", subP->attrsFormat));
    }
    else if (SCOMPARE9(itemP->name, 'e', 'n', 'd', 'p', 'o', 'i', 'n', 't', 0))
    {
      DUPLICATE_CHECK(endpointP, "Notification::endpoint", itemP);
      OBJECT_CHECK(itemP, "Notification::endpoint");

      if (ktreeToEndpoint(ciP, itemP, &subP->notification.httpInfo) == false)
        return false;
    }
    else if (SCOMPARE7(itemP->name, 's', 't', 'a', 't', 'u', 's', 0))
    {
      // Ignored field - internal field that cannot be set by requests
    }
    else if (SCOMPARE10(itemP->name, 't', 'i', 'm', 'e', 's', 'S', 'e', 'n', 't', 0))
    {
      // Ignored field - internal field that cannot be set by requests
    }
    else if (SCOMPARE17(itemP->name, 'l', 'a', 's', 't', 'N', 'o', 't', 'i', 'f', 'i', 'c', 'a', 't', 'i', 'o', 'n', 0))
    {
      // Ignored field - internal field that cannot be set by requests
    }
    else if (SCOMPARE12(itemP->name, 'l', 'a', 's', 't', 'F', 'a', 'i', 'l', 'u', 'r', 'e', 0))
    {
      // Ignored field - internal field that cannot be set by requests
    }
    else if (SCOMPARE12(itemP->name, 'l', 'a', 's', 't', 'S', 'u', 'c', 'c', 'e', 's', 's', 0))
    {
      // Ignored field - internal field that cannot be set by requests
    }
    else
    {
      orionldErrorResponseCreate(ciP, OrionldBadRequestData, "Unknown Notification field", itemP->name, OrionldDetailsString);
      return false;
    }
  }

  return true;
}




// -----------------------------------------------------------------------------
//
// ktreeToSubscription -
//
static bool ktreeToSubscription(ConnectionInfo* ciP, ngsiv2::Subscription* subP)
{
  KjNode*                   kNodeP;
  char*                     idP                       = NULL;
  char*                     typeP                     = NULL;
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
  long long                 throttling                = -1;
  bool                      throttlingPresent         = false;
  KjNode*                   idNodeP                   = NULL;
  KjNode*                   contextNodeP              = NULL;

  // Default values
  subP->attrsFormat = NGSI_LD_V1_KEYVALUES;
  subP->descriptionProvided = false;
  subP->expires = 0;
  subP->throttling = 0;
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
  // 1. First lookup the tree nodes of 'id' and '@context'
  // 2. Then create a context attribute for the context - FIXME: combine code with POST Entities
  //
  for (kNodeP = ciP->requestTree->value.firstChildP; kNodeP != NULL; kNodeP = kNodeP->next)
  {
    if (SCOMPARE9(kNodeP->name, '@', 'c', 'o', 'n', 't', 'e', 'x', 't', 0))
    {
      contextNodeP = kNodeP;
      if (idNodeP != NULL)
        break;
    }
    else if (SCOMPARE3(kNodeP->name, 'i', 'd', 0))
    {
      idNodeP = kNodeP;
      if (contextNodeP != NULL)
        break;
    }
  }

  if (idNodeP != NULL)
  {
    STRING_CHECK(idNodeP, "Subscription::id");

    // Must be a URI
    if ((urlCheck(idNodeP->value.s, NULL) == false) && (urnCheck(idNodeP->value.s, NULL) == false))
    {
      LM_E(("Subscription::id is not a URI"));
      orionldErrorResponseCreate(ciP, OrionldBadRequestData, "Subscription::id is not a URI", idNodeP->value.s, OrionldDetailsAttribute);
      return false;
    }

    subP->id = idNodeP->value.s;
  }
  else
  {
    char randomId[32];

    mongoIdentifier(randomId);

    subP->id  = "urn:ngsi-ld:Subscription:";
    subP->id += randomId;
  }

  if (ciP->httpHeaders.ngsildContent)  // Context in payload
  {
    if (contextNodeP == NULL)
    {
      // error
      return false;
    }

    
    if (orionldContextTreat(ciP, contextNodeP, (char*) subP->id.c_str(), NULL) == false)
    {
      // error
      return false;
    }
  }

#if 0
  else if (ciP->httpHeaders.linkUrl != NULL)  // Context in Link HTTP Header
  {
    ...
  }
#endif

  // Add caP
  
  //
  // Now loop over the tree
  //
  for (kNodeP = ciP->requestTree->value.firstChildP; kNodeP != NULL; kNodeP = kNodeP->next)
  {
    if (SCOMPARE3(kNodeP->name, 'i', 'd', 0))
    {
      DUPLICATE_CHECK(idP, "Subscription::id", kNodeP->value.s);
      STRING_CHECK(kNodeP, "Subscription::id");
      subP->id = kNodeP->value.s;
    }
    else if (SCOMPARE5(kNodeP->name, 't', 'y', 'p', 'e', 0))
    {
      //
      // NOTE
      //   The spec of ngsi-ld states that the field "type" is MANDATORY and MUST be set to "Subscription".
      //   A bit funny in my opinion.
      //   However, here we make sure that the spec is followed, but we add nothing to the database.
      //   When rendering (serializing) subscriptions for GET /subscriptions, the field
      //     "type": "Subscription"
      //   is added to the response payload.
      //
      DUPLICATE_CHECK(typeP, "Subscription::type", kNodeP->value.s);
      STRING_CHECK(kNodeP, "Subscription::type");

      if (!SCOMPARE13(kNodeP->value.s, 'S', 'u', 'b', 's', 'c', 'r', 'i', 'p', 't', 'i', 'o', 'n', 0))
      {
        orionldErrorResponseCreate(ciP, OrionldBadRequestData, "Invalid value for Subscription::type", kNodeP->value.s, OrionldDetailsString);
        return false;
      }
    }
    else if (SCOMPARE5(kNodeP->name, 'n', 'a', 'm', 'e', 0))
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

      if (ktreeToEntities(ciP, entitiesP, &subP->subject.entities) == false)
      {
        LM_E(("ktreeToEntities failed"));
        return false;  // orionldErrorResponseCreate is invoked by ktreeToEntities
      }
    }
    else if (SCOMPARE18(kNodeP->name, 'w', 'a', 't', 'c', 'h', 'e', 'd', 'A', 't', 't', 'r', 'i', 'b', 'u', 't', 'e', 's', 0))
    {
      DUPLICATE_CHECK_WITH_PRESENCE(watchedAttributesPresent, watchedAttributesP, "Subscription::watchedAttributes", kNodeP);
      ARRAY_CHECK(kNodeP, "Subscription::watchedAttributes");
      EMPTY_ARRAY_CHECK(kNodeP,"Subscription::watchedAttributes");

      if (ktreeToStringList(ciP, watchedAttributesP, &subP->subject.condition.attributes) == false)
      {
        LM_E(("ktreeToStringList failed"));
        return false;  // orionldErrorResponseCreate is invoked by ktreeToStringList
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

      // FIXME: Need to parse the Q and to URI Expansion
      subP->subject.condition.expression.q = kNodeP->value.s;
    }
    else if (SCOMPARE5(kNodeP->name, 'g', 'e', 'o', 'Q', 0))
    {
      DUPLICATE_CHECK_WITH_PRESENCE(geoQPresent, geoQP, "Subscription::geoQ", kNodeP);
      OBJECT_CHECK(kNodeP, "Subscription::geoQ");

      if (ktreeToSubscriptionExpression(ciP, geoQP, &subP->subject.condition.expression) == false)
      {
        LM_E(("ktreeToSubscriptionExpression failed"));
        return false;  // orionldErrorResponseCreate is invoked by ktreeToSubscriptionExpression
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

      if (ktreeToNotification(ciP, notificationP, subP) == false)
      {
        LM_E(("ktreeToNotification failed"));
        return false;  // orionldErrorResponseCreate is invoked by ktreeToNotification
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
    else if (SCOMPARE9(kNodeP->name, '@', 'c', 'o', 'n', 't', 'e', 'x', 't', 0))
    {
      STRING_CHECK(kNodeP, "Subscription::@context");
      subP->ldContext = kNodeP->value.s;
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



// ----------------------------------------------------------------------------
//
// orionldPostSubscriptions -
//
// A ngsi-ld subscription contains the following fields:
// - id                 Subscription::id                                              (URI given by creating request or auto-generated)
// - type               Not in DB                                                     (must be "Subscription" - will not be saved in mongo)
// - name               NOT SUPPORTED                                                 (String)
// - description        Subscription::description                                     (String)
// - entities           Subscription::Subject::entities                               (Array of EntityInfo which is a subset of EntID)
// - watchedAttributes  Subscription::Notification::attributes                        (Array of String)
// - timeInterval       NOT SUPPORTED                                                 (will not be implemented any time soon - not very useful)
// - q                  Subscription::Subject::Condition::SubscriptionExpression::q
// - geoQ               NOT SUPPORTED
// - csf                NOT SUPPORTED
// - isActive           May not be necessary to store in mongo - "status" is enough?
// - notification       Subscription::Notification + Subscription::attrsFormat?
// - expires            Subscription::expires                                         (DateTime)
// - throttling         Subscription::throttling                                      (Number - in seconds)
// - status             Subscription::status                                          (builtin String: "active", "paused", "expired")
//
// * At least one of 'entities' and 'watchedAttributes' must be present.
// * Either 'timeInterval' or 'watchedAttributes' must be present. But not both of them
// * For now, 'timeInterval' will not be implemented. If ever ...
//
bool orionldPostSubscriptions(ConnectionInfo* ciP)
{
  ngsiv2::Subscription sub;
  std::string          subId;
  OrionError           oError;

  LM_T(LmtServiceRoutine, ("In orionldPostSubscriptions - calling ktreeToSubscription"));

  // FIXME: attrsFormat should be set to default by constructor
  sub.attrsFormat = DEFAULT_RENDER_FORMAT;  // FIXME: 

  if (ktreeToSubscription(ciP, &sub) == false)
  {
    LM_E(("ktreeToSubscription FAILED"));
    // orionldErrorResponseCreate is invoked by ktreeToSubscription
    return false;
  }

  subId = mongoCreateSubscription(sub,
                                  &oError,
                                  ciP->httpHeaders.tenant,
                                  ciP->servicePathV,
                                  ciP->httpHeaders.xauthToken,
                                  ciP->httpHeaders.correlator,
                                  sub.ldContext);

  ciP->httpStatusCode = SccCreated;
  httpHeaderLocationAdd(ciP, "/ngsi-ld/v1/subscriptions/", subId.c_str());
  httpHeaderLinkAdd(ciP, ciP->contextP, NULL);

  return true;
}
