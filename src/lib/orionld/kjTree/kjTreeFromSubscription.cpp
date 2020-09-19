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
#include <strings.h>                                             // bzero

extern "C"
{
#include "kalloc/kaAlloc.h"                                      // kaAlloc
#include "kjson/KjNode.h"                                        // KjNode
#include "kjson/kjBuilder.h"                                     // kjObject, kjString, kjBoolean, ...
#include "kjson/kjParse.h"                                       // kjParse
}

#include "logMsg/logMsg.h"                                       // LM_*
#include "logMsg/traceLevels.h"                                  // Lmt*

#include "apiTypesV2/Subscription.h"                             // Subscription
#include "orionld/common/numberToDate.h"                         // numberToDate
#include "orionld/common/orionldErrorResponse.h"                 // orionldErrorResponseCreate, OrionldInternalError
#include "orionld/common/orionldState.h"                         // orionldState
#include "orionld/common/qAliasCompact.h"                        // qAliasCompact
#include "orionld/common/eqForDot.h"                             // eqForDot
#include "orionld/context/OrionldContext.h"                      // OrionldContext
#include "orionld/context/orionldCoreContext.h"                  // orionldCoreContext
#include "orionld/context/orionldContextCacheLookup.h"           // orionldContextCacheLookup
#include "orionld/context/orionldContextItemAliasLookup.h"       // orionldContextItemAliasLookup
#include "orionld/kjTree/kjTreeFromSubscription.h"               // Own interface



// -----------------------------------------------------------------------------
//
// coordinateTransform -
//
void coordinateTransform(const char* geometry, char* to, int toLen, char* from)
{
  if ((strcmp(geometry, "Point") == 0) || (strcmp(geometry, "Polygon") == 0))
  {
    snprintf(to, toLen, "[%s]", from);
    return;
  }
#if 0
  int toIx = 0;

  to[0] = '[';
  ++toIx;

  while (*from != 0)
  {
    if (*from == ';')
    {
      to[toIx++] = ']';
      to[toIx++] = ',';
      to[toIx++] = '[';
    }
    else
    {
      to[toIx++] = *from;
    }
    ++from;
  }

  to[toIx] = ']';
#endif
}



// -----------------------------------------------------------------------------
//
// kjTreeFromSubscription -
//
KjNode* kjTreeFromSubscription(ngsiv2::Subscription* subscriptionP)
{
  KjNode*          topP = kjObject(orionldState.kjsonP, NULL);
  KjNode*          objectP;
  KjNode*          arrayP;
  KjNode*          nodeP;
  unsigned int     size;
  unsigned int     ix;
  OrionldContext*  contextP = orionldContextCacheLookup(subscriptionP->ldContext.c_str());

  // id
  nodeP = kjString(orionldState.kjsonP, "id", subscriptionP->id.c_str());
  kjChildAdd(topP, nodeP);


  // type
  nodeP = kjString(orionldState.kjsonP, "type", "Subscription");
  kjChildAdd(topP, nodeP);


  // name
  if (subscriptionP->name != "")
  {
    nodeP = kjString(orionldState.kjsonP, "name", subscriptionP->name.c_str());
    kjChildAdd(topP, nodeP);
  }


  // description
  if (subscriptionP->description != "")
  {
    nodeP  = kjString(orionldState.kjsonP, "description", subscriptionP->description.c_str());
    kjChildAdd(topP, nodeP);
  }

  // sysAttrs - createdAt and modifiedAt
  if (orionldState.uriParamOptions.sysAttrs == true)
  {
    char    dateTime[64];
    char*   detail;
    KjNode* nodeP;

    if (subscriptionP->createdAt != -1)
    {
      numberToDate(subscriptionP->createdAt, dateTime, sizeof(dateTime), &detail);
      nodeP = kjString(orionldState.kjsonP, "createdAt", dateTime);
      kjChildAdd(topP, nodeP);
    }

    if (subscriptionP->modifiedAt != -1)
    {
      numberToDate(subscriptionP->modifiedAt, dateTime, sizeof(dateTime), &detail);
      nodeP = kjString(orionldState.kjsonP, "modifiedAt", dateTime);
      kjChildAdd(topP, nodeP);
    }
  }


  // entities
  size = subscriptionP->subject.entities.size();
  if (size != 0)
  {
    arrayP = kjArray(orionldState.kjsonP, "entities");

    for (ix = 0; ix < size; ix++)
    {
      ngsiv2::EntID* eP = &subscriptionP->subject.entities[ix];

      objectP = kjObject(orionldState.kjsonP, NULL);

      if (eP->id != "")
      {
        nodeP = kjString(orionldState.kjsonP, "id", eP->id.c_str());
        kjChildAdd(objectP, nodeP);
      }

      if ((eP->idPattern != "") && (eP->idPattern != ".*"))
      {
        nodeP = kjString(orionldState.kjsonP, "idPattern", eP->idPattern.c_str());
        kjChildAdd(objectP, nodeP);
      }

      char* alias = orionldContextItemAliasLookup(contextP, eP->type.c_str(), NULL, NULL);

      nodeP = kjString(orionldState.kjsonP, "type", alias);
      kjChildAdd(objectP, nodeP);
      kjChildAdd(arrayP, objectP);
    }
    kjChildAdd(topP, arrayP);
  }


  // watchedAttributes
  size = subscriptionP->subject.condition.attributes.size();
  if (size > 0)
  {
    // watchedAttributesPresent = true;
    arrayP = kjArray(orionldState.kjsonP, "watchedAttributes");

    for (ix = 0; ix < size; ix++)
    {
      char* attrName = (char*) subscriptionP->subject.condition.attributes[ix].c_str();
      char* alias    = orionldContextItemAliasLookup(contextP, attrName, NULL, NULL);

      nodeP = kjString(orionldState.kjsonP, NULL, alias);
      kjChildAdd(arrayP, nodeP);
    }
    kjChildAdd(topP, arrayP);
  }


  // timeInterval (it's not in use ...)
  if ((subscriptionP->timeInterval != -1) && (subscriptionP->timeInterval != 0))
  {
    nodeP = kjInteger(orionldState.kjsonP, "timeInterval", subscriptionP->timeInterval);
    kjChildAdd(topP, nodeP);
  }


  // q
  const char* q = subscriptionP->subject.condition.expression.q.c_str();
  if (q[0] != 0)
  {
    nodeP = kjString(orionldState.kjsonP, "q", q);
    qAliasCompact(nodeP, true);
    kjChildAdd(topP, nodeP);
  }


  // geoQ
  if (subscriptionP->subject.condition.expression.geometry != "")
  {
    objectP = kjObject(orionldState.kjsonP, "geoQ");

    nodeP = kjString(orionldState.kjsonP, "geometry", subscriptionP->subject.condition.expression.geometry.c_str());
    kjChildAdd(objectP, nodeP);

    if ((subscriptionP->subject.condition.expression.geometry == "Point") || (subscriptionP->subject.condition.expression.geometry == "Polygon"))
    {
      //
      // The "coordinates" have been saved as a string but should be a json array.
      // Easiest way is to parse the string and simply add the output tree as value of "coordinates"
      //
      char*    coordinateString    = (char*) subscriptionP->subject.condition.expression.coords.c_str();
      int      coordinateVectorLen = strlen(coordinateString) * 4 + 10;
      char*    coordinateVector    = (char*) malloc(coordinateVectorLen);
      KjNode*  coordValueP;

      coordinateTransform(subscriptionP->subject.condition.expression.geometry.c_str(), coordinateVector, coordinateVectorLen, coordinateString);

      coordValueP = kjParse(orionldState.kjsonP, coordinateVector);
      free(coordinateVector);

      if (coordValueP == NULL)
      {
        LM_E(("error parsing coordinates: '%s'", coordinateString));
        orionldErrorResponseCreate(OrionldInternalError, "Unable to parse coordinates string", coordinateString);
        return NULL;
      }

      coordValueP->name = (char*) "coordinates";

      kjChildAdd(objectP, coordValueP);
    }
    else
    {
      nodeP = kjString(orionldState.kjsonP, "coordinates", subscriptionP->subject.condition.expression.coords.c_str());
      kjChildAdd(objectP, nodeP);
    }

    nodeP = kjString(orionldState.kjsonP, "georel", subscriptionP->subject.condition.expression.georel.c_str());
    kjChildAdd(objectP, nodeP);

    if (subscriptionP->subject.condition.expression.geoproperty != "")
    {
      char* geoproperty = (char*) subscriptionP->subject.condition.expression.geoproperty.c_str();

      eqForDot(geoproperty);
      geoproperty = orionldContextItemAliasLookup(orionldState.contextP, geoproperty, NULL, NULL);
      nodeP       = kjString(orionldState.kjsonP, "geoproperty", geoproperty);
      kjChildAdd(objectP, nodeP);
    }

    kjChildAdd(topP, objectP);
  }

  // csf
  if (subscriptionP->csf != "")
  {
    nodeP = kjString(orionldState.kjsonP, "csf", subscriptionP->csf.c_str());
    kjChildAdd(topP, nodeP);
  }

  // isActive
  bool isActive = (subscriptionP->status == "active")? true : false;
  if (isActive == false)
  {
    nodeP = kjBoolean(orionldState.kjsonP, "isActive", false);
    kjChildAdd(topP, nodeP);
  }

  // status
  double  now    = getCurrentTime();
  char*   status = NULL;

  if (subscriptionP->expires < now)
    status = (char*) "expired";
  else if (subscriptionP->status != "active")
    status = (char*) "paused";

  if (status != NULL)
  {
    nodeP = kjString(orionldState.kjsonP, "status", status);
    kjChildAdd(topP, nodeP);
  }

  // notification
  objectP = kjObject(orionldState.kjsonP, "notification");

  // notification::attributes
  size = subscriptionP->notification.attributes.size();
  if (size > 0)
  {
    arrayP = kjArray(orionldState.kjsonP, "attributes");
    for (ix = 0; ix < size; ++ix)
    {
      char* attrName = (char*) subscriptionP->notification.attributes[ix].c_str();
      char* alias    = orionldContextItemAliasLookup(contextP, attrName, NULL, NULL);

      nodeP = kjString(orionldState.kjsonP, NULL, alias);
      kjChildAdd(arrayP, nodeP);
    }
    kjChildAdd(objectP, arrayP);
  }

  // notification::format
  if (subscriptionP->attrsFormat == NGSI_V2_KEYVALUES)
    nodeP = kjString(orionldState.kjsonP, "format", "keyValues");
  else
    nodeP = kjString(orionldState.kjsonP, "format", "normalized");
  kjChildAdd(objectP, nodeP);

  // notification::endpoint
  KjNode* endpointP = kjObject(orionldState.kjsonP, "endpoint");

  nodeP = kjString(orionldState.kjsonP, "uri", subscriptionP->notification.httpInfo.url.c_str());
  kjChildAdd(endpointP, nodeP);

  const char* mimeType = (subscriptionP->notification.httpInfo.mimeType == JSON)? "application/json" : "application/ld+json";
  char        dateTime[128];
  char*       detail;

  nodeP = kjString(orionldState.kjsonP, "accept", mimeType);
  kjChildAdd(endpointP, nodeP);

  kjChildAdd(objectP, endpointP);


  // notification::timesSent
  if (subscriptionP->notification.timesSent > 0)
  {
    nodeP = kjInteger(orionldState.kjsonP, "timesSent", subscriptionP->notification.timesSent);
    kjChildAdd(objectP, nodeP);
  }

  // notification::lastNotification
  if (subscriptionP->notification.lastNotification > 0)
  {
    numberToDate(subscriptionP->notification.lastNotification, dateTime, sizeof(dateTime), &detail);
    nodeP = kjString(orionldState.kjsonP, "lastNotification", dateTime);
    kjChildAdd(objectP, nodeP);
  }

  // notification::lastFailure
  if (subscriptionP->notification.lastFailure > 0)
  {
    numberToDate(subscriptionP->notification.lastFailure, dateTime, sizeof(dateTime), &detail);
    nodeP = kjString(orionldState.kjsonP, "lastFailure", dateTime);
    kjChildAdd(objectP, nodeP);
  }

  // notification::lastSuccess
  if (subscriptionP->notification.lastSuccess > 0)
  {
    numberToDate(subscriptionP->notification.lastSuccess, dateTime, sizeof(dateTime), &detail);
    nodeP = kjString(orionldState.kjsonP, "lastSuccess", dateTime);
    kjChildAdd(objectP, nodeP);
  }

  kjChildAdd(topP, objectP);


  // expires
  if (subscriptionP->expires != 0x7FFFFFFF)
  {
    char*            details;
    char             date[64];

    if (numberToDate(subscriptionP->expires, date, sizeof(date), &details) == false)
    {
      LM_E(("Error creating a stringified date for 'expires'"));
      orionldErrorResponseCreate(OrionldInternalError, "Unable to create a stringified expires date", details);
      return NULL;
    }
    nodeP = kjString(orionldState.kjsonP, "expires", date);
    kjChildAdd(topP, nodeP);
  }

  // throttling
  if (subscriptionP->throttling != 0)
  {
    nodeP = kjFloat(orionldState.kjsonP, "throttling", subscriptionP->throttling);
    kjChildAdd(topP, nodeP);
  }

  // @context - in payload if Mime Type is application/ld+json, else in Link header
  if (orionldState.acceptJsonld)
  {
    if (subscriptionP->ldContext != "")
      nodeP = kjString(orionldState.kjsonP, "@context", subscriptionP->ldContext.c_str());
    else
      nodeP = kjString(orionldState.kjsonP, "@context", orionldCoreContextP->url);

    kjChildAdd(topP, nodeP);
  }

  return topP;
}
