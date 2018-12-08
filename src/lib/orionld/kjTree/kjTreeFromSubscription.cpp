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
extern "C"
{
#include "kjson/KjNode.h"                                      // KjNode
#include "kjson/kjBuilder.h"                                   // kjObject, kjString, kjBoolean, ...
}

#include "logMsg/logMsg.h"                                     // LM_*
#include "logMsg/traceLevels.h"                                // Lmt*

#include "rest/ConnectionInfo.h"                               // ConnectionInfo
#include "rest/httpHeaderAdd.h"                                // httpHeaderAdd, httpHeaderLinkAdd
#include "apiTypesV2/Subscription.h"                           // Subscription
#include "orionld/context/orionldCoreContext.h"                // orionldDefaultContext
#include "orionld/common/numberToDate.h"                       // numberToDate
#include "orionld/common/orionldErrorResponse.h"               // orionldErrorResponseCreate, OrionldInternalError
#include "orionld/kjTree/kjTreeFromSubscription.h"             // Own interface



// -----------------------------------------------------------------------------
//
// kjTreeFromSubscription -
//
KjNode* kjTreeFromSubscription(ConnectionInfo* ciP, ngsiv2::Subscription* subscriptionP)
{
  KjNode*       topP = kjObject(NULL, NULL);
  KjNode*       objectP;
  KjNode*       arrayP;
  KjNode*       nodeP;
  char*         details;
  bool          watchedAttributesPresent = false;
  char          date[64];
  unsigned int  size;
  unsigned int  ix;

  // id
  nodeP = kjString(ciP->kjsonP, "id", subscriptionP->id.c_str());
  kjChildAdd(topP, nodeP);


  // type
  nodeP = kjString(ciP->kjsonP, "type", "Subscription");
  kjChildAdd(topP, nodeP);


  // name
  if (subscriptionP->name != "")
  {
    nodeP = kjString(ciP->kjsonP, "name", subscriptionP->name.c_str());
    kjChildAdd(topP, nodeP);
  }


  // description
  if ((subscriptionP->descriptionProvided) && (subscriptionP->description != ""))
  {
    nodeP  = kjString(ciP->kjsonP, "description", subscriptionP->description.c_str());
    kjChildAdd(topP, nodeP);
  }

  // entities
  size = subscriptionP->subject.entities.size();
  if (size != 0)
  {
    arrayP = kjArray(ciP->kjsonP, "entities");
    for (ix = 0; ix < size; ix++)
    {
      ngsiv2::EntID* eP = &subscriptionP->subject.entities[ix];

      objectP = kjObject(ciP->kjsonP, NULL);

      if (eP->id != "")
      {
        nodeP = kjString(ciP->kjsonP, "id", eP->id.c_str());
        kjChildAdd(objectP, nodeP);
      }

      if (eP->idPattern != "")
      {
        nodeP = kjString(ciP->kjsonP, "idPattern", eP->idPattern.c_str());
        kjChildAdd(objectP, nodeP);
      }

      nodeP = kjString(ciP->kjsonP, "type", eP->type.c_str());
      kjChildAdd(objectP, nodeP);

      kjChildAdd(arrayP, objectP);
    }
    kjChildAdd(topP, arrayP);
  }


  // watchedAttributes
  size = subscriptionP->subject.condition.attributes.size();
  if (size > 0)
  {
    watchedAttributesPresent = true;
    arrayP = kjArray(ciP->kjsonP, "watchedAttributes");

    for (ix = 0; ix < size; ix++)
    {
      nodeP = kjString(ciP->kjsonP, NULL, subscriptionP->subject.condition.attributes[ix].c_str());
      kjChildAdd(arrayP, nodeP);
    }
    kjChildAdd(topP, arrayP);
  }


  // timeInterval - FIXME: Implement?
  if (watchedAttributesPresent == false)
  {
#if 0
    if (numberToDate((time_t) subscriptionP->timeInterval, date, sizeof(date), &details) == false)
    {
      LM_E(("Error creating a stringified date for 'timeInterval'"));
      orionldErrorResponseCreate(ciP, OrionldInternalError, "unable to create a stringified date", details, OrionldDetailsEntity);
      return false;
    }
    nodeP = kjString(ciP->kjsonP, "timeInterval", date);
    kjChildAdd(topP, nodeP);
#endif
  }


  // q
  const char* q = subscriptionP->subject.condition.expression.q.c_str();
  if (q[0] != 0)
  {
    nodeP = kjString(ciP->kjsonP, "q", q);
    kjChildAdd(topP, nodeP);
  }


  // geoQ
  if (subscriptionP->subject.condition.expression.geometry != "")
  {
    objectP = kjObject(ciP->kjsonP, "geoQ");

    nodeP = kjString(ciP->kjsonP, "geometry", subscriptionP->subject.condition.expression.geometry.c_str());
    kjChildAdd(objectP, nodeP);

    nodeP = kjString(ciP->kjsonP, "coordinates", subscriptionP->subject.condition.expression.coords.c_str());
    kjChildAdd(objectP, nodeP);

    nodeP = kjString(ciP->kjsonP, "georel", subscriptionP->subject.condition.expression.georel.c_str());
    kjChildAdd(objectP, nodeP);

    // FIXME: geoproperty not supported for now
    kjChildAdd(topP, objectP);
  }

  // csf - FIXME: Implement!

  // isActive
  bool isActive = (subscriptionP->status == "active")? true : false;
  nodeP = kjBoolean(ciP->kjsonP, "isActive", isActive);
  kjChildAdd(topP, nodeP);

  // notification
  objectP = kjObject(ciP->kjsonP, "notification");

  // notification::attributes
  size = subscriptionP->notification.attributes.size();
  if (size > 0)
  {
    arrayP = kjArray(ciP->kjsonP, "attributes");
    for (ix = 0; ix < size; ++ix)
    {
      nodeP = kjString(ciP->kjsonP, NULL, subscriptionP->notification.attributes[ix].c_str());
      kjChildAdd(arrayP, nodeP);
    }
    kjChildAdd(objectP, arrayP);
  }

  // notification::format
  if (subscriptionP->attrsFormat == NGSI_V2_KEYVALUES)
    nodeP = kjString(ciP->kjsonP, "format", "keyValues");
  else
    nodeP = kjString(ciP->kjsonP, "format", "normalized");
  kjChildAdd(objectP, nodeP);

  // notification::endpoint
  KjNode* endpointP = kjObject(ciP->kjsonP, "endpoint");

  nodeP = kjString(ciP->kjsonP, "uri", subscriptionP->notification.httpInfo.url.c_str());
  kjChildAdd(endpointP, nodeP);

  const char* mimeType = (subscriptionP->notification.httpInfo.mimeType == JSON)? "application/json" : "application/ld+json";
  nodeP = kjString(ciP->kjsonP, "accept", mimeType);
  kjChildAdd(endpointP, nodeP);

  kjChildAdd(objectP, endpointP);


  // notification::timesSent
  if (subscriptionP->notification.timesSent > 0)
  {
    nodeP = kjInteger(ciP->kjsonP, "timesSent", subscriptionP->notification.timesSent);
    kjChildAdd(objectP, nodeP);
  }

  // notification::lastNotification
  if (subscriptionP->notification.lastNotification > 0)
  {
    nodeP = kjInteger(ciP->kjsonP, "lastNotification", subscriptionP->notification.lastNotification);
    kjChildAdd(objectP, nodeP);
  }

  // notification::lastFailure
  if (subscriptionP->notification.lastFailure > 0)
  {
    nodeP = kjInteger(ciP->kjsonP, "lastFailure", subscriptionP->notification.lastFailure);
    kjChildAdd(objectP, nodeP);
  }

  // notification::lastSuccess
  if (subscriptionP->notification.lastSuccess > 0)
  {
    nodeP = kjInteger(ciP->kjsonP, "lastSuccess", subscriptionP->notification.lastSuccess);
    kjChildAdd(objectP, nodeP);
  }

  kjChildAdd(topP, objectP);


  // expires
  if (numberToDate((time_t) subscriptionP->expires, date, sizeof(date), &details) == false)
  {
    LM_E(("Error creating a stringified date for 'expires'"));
    orionldErrorResponseCreate(ciP, OrionldInternalError, "unable to create a stringified date", details, OrionldDetailsEntity);
    return NULL;
  }
  nodeP = kjString(ciP->kjsonP, "expires", date);
  kjChildAdd(topP, nodeP);

  // throttling
  nodeP = kjInteger(ciP->kjsonP, "throttling", subscriptionP->throttling);
  kjChildAdd(topP, nodeP);

  // status
  nodeP = kjString(ciP->kjsonP, "status", subscriptionP->status.c_str());
  kjChildAdd(topP, nodeP);

  // @context - in payload if Mime Type is application/ld+json, else in Link header
  if (ciP->httpHeaders.acceptJsonld)
  {
    if (subscriptionP->ldContext != "")
      nodeP = kjString(ciP->kjsonP, "@context", subscriptionP->ldContext.c_str());
    else
      nodeP = kjString(ciP->kjsonP, "@context", orionldDefaultContext.url);

    kjChildAdd(topP, nodeP);
  }

  return topP;
}
