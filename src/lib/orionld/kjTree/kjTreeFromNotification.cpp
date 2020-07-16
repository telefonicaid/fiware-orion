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
#include "kjson/KjNode.h"                                        // KjNode
#include "kjson/kjBuilder.h"                                     // kjObject, kjString, kjBoolean, ...
}

#include "logMsg/logMsg.h"                                       // LM_*
#include "logMsg/traceLevels.h"                                  // Lmt*

#include "ngsi10/NotifyContextRequest.h"                         // NotifyContextRequest
#include "mongoBackend/MongoGlobal.h"                            // mongoIdentifier

#include "common/RenderFormat.h"                                 // RenderFormat
#include "orionld/common/orionldState.h"                         // orionldState
#include "orionld/common/numberToDate.h"                         // numberToDate
#include "orionld/common/SCOMPARE.h"                             // SCOMPAREx
#include "orionld/context/OrionldContext.h"                      // OrionldContext
#include "orionld/context/orionldCoreContext.h"                  // ORIONLD_CORE_CONTEXT_URL
#include "orionld/context/orionldContextItemAliasLookup.h"       // orionldContextItemAliasLookup
#include "orionld/context/orionldContextCacheLookup.h"           // orionldContextCacheLookup
#include "orionld/kjTree/kjTreeFromContextAttribute.h"           // kjTreeFromContextAttribute
#include "orionld/kjTree/kjTreeFromNotification.h"               // Own interface



// -----------------------------------------------------------------------------
//
// kjTreeFromNotification -
//
KjNode* kjTreeFromNotification(NotifyContextRequest* ncrP, const char* context, MimeType mimeType, RenderFormat renderFormat, char** detailsP)
{
  KjNode*          nodeP;
  char             buf[32];
  KjNode*          rootP      = kjObject(orionldState.kjsonP, NULL);
  char*            id         = mongoIdentifier(buf);
  char             idBuffer[] = "urn:ngsi-ld:Notification:012345678901234567890123";  // The 012345678901234567890123 will be overwritten
  OrionldContext*  contextP   = orionldContextCacheLookup(context);

  // id
  strcpy(&idBuffer[25], id);
  nodeP = kjString(orionldState.kjsonP, "id", idBuffer);
  kjChildAdd(rootP, nodeP);

  // type
  nodeP = kjString(orionldState.kjsonP, "type", "Notification");
  kjChildAdd(rootP, nodeP);

  // subscriptionId
  nodeP = kjString(orionldState.kjsonP, "subscriptionId", (char*) ncrP->subscriptionId.get().c_str());
  kjChildAdd(rootP, nodeP);

  // context - if JSONLD
  if (mimeType == JSONLD)
  {
    if (context == NULL)
      nodeP = kjString(orionldState.kjsonP, "@context", ORIONLD_CORE_CONTEXT_URL);
    else
      nodeP = kjString(orionldState.kjsonP, "@context", context);

    kjChildAdd(rootP, nodeP);
  }
  else
  {
    // Context for HTTP Link Header is done in Notifier::buildSenderParams (Notifier.cpp)
  }

  // notifiedAt
  double  now = getCurrentTime();  // FIXME - use an already existing timestamp!
  char    date[128];
  char*   details;

  if (numberToDate(now, date, sizeof(date), &details) == false)
  {
    LM_E(("Runtime Error (numberToDate: %s)", details));
    return NULL;
  }
  nodeP = kjString(orionldState.kjsonP, "notifiedAt", date);
  kjChildAdd(rootP, nodeP);


  //
  // data
  //
  KjNode* dataP = kjArray(orionldState.kjsonP, "data");
  kjChildAdd(rootP, dataP);

  //
  // loop over ContextElements in NotifyContextRequest::contextElementResponseVector
  //
  for (unsigned int ix = 0; ix < ncrP->contextElementResponseVector.size(); ix++)
  {
    ContextElement* ceP     = &ncrP->contextElementResponseVector[ix]->contextElement;
    KjNode*         objectP = kjObject(orionldState.kjsonP, NULL);
    char*           alias;
    KjNode*         nodeP;

    kjChildAdd(dataP, objectP);

    // entity id - Mandatory URI
    nodeP = kjString(orionldState.kjsonP, "id", ceP->entityId.id.c_str());
    kjChildAdd(objectP, nodeP);

    // entity type - Mandatory URI
    alias = orionldContextItemAliasLookup(contextP, ceP->entityId.type.c_str(), NULL, NULL);
    nodeP = kjString(orionldState.kjsonP, "type", alias);
    kjChildAdd(objectP, nodeP);

    // Attributes
    for (unsigned int aIx = 0; aIx < ceP->contextAttributeVector.size(); aIx++)
    {
      ContextAttribute*  aP       = ceP->contextAttributeVector[aIx];
      const char*        attrName = aP->name.c_str();

      if (SCOMPARE9(attrName, '@', 'c', 'o', 'n', 't', 'e', 'x', 't', 0))
        continue;

      nodeP = kjTreeFromContextAttribute(aP, contextP, renderFormat, detailsP);
      kjChildAdd(objectP, nodeP);
    }
    // location                        GeoProperty
    // observationSpace                GeoProperty
    // operationSpace                  GeoProperty
    // Property 0..N
    // Relationship 0..N
    //
  }

  return rootP;
}
