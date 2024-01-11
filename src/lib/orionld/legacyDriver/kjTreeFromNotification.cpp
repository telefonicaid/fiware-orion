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
#include <unistd.h>                                            // NULL

extern "C"
{
#include "kjson/KjNode.h"                                      // KjNode
#include "kjson/kjBuilder.h"                                   // kjObject, kjString, kjBoolean, ...
#include "kjson/kjLookup.h"                                    // kjLookup
}

#include "logMsg/logMsg.h"                                     // LM_*

#include "ngsi10/NotifyContextRequest.h"                       // NotifyContextRequest

#include "orionld/types/OrionldContext.h"                      // OrionldContext
#include "orionld/types/OrionldRenderFormat.h"                 // OrionldRenderFormat
#include "orionld/common/orionldState.h"                       // orionldState, coreContextUrl
#include "orionld/common/numberToDate.h"                       // numberToDate
#include "orionld/common/SCOMPARE.h"                           // SCOMPAREx
#include "orionld/common/uuidGenerate.h"                       // uuidGenerate
#include "orionld/context/orionldContextItemAliasLookup.h"     // orionldContextItemAliasLookup
#include "orionld/contextCache/orionldContextCacheLookup.h"    // orionldContextCacheLookup
#include "orionld/kjTree/kjEntityNormalizedToConcise.h"        // kjEntityNormalizedToConcise
#include "orionld/legacyDriver/kjTreeFromContextAttribute.h"   // kjTreeFromContextAttribute
#include "orionld/legacyDriver/kjTreeFromNotification.h"       // Own interface



// -----------------------------------------------------------------------------
//
// kjTreeFromNotification -
//
KjNode* kjTreeFromNotification(NotifyContextRequest* ncrP, const char* context, MimeType mimeType, OrionldRenderFormat renderFormat, const char* lang, char** detailsP)
{
  KjNode*          nodeP;
  KjNode*          rootP             = kjObject(orionldState.kjsonP, NULL);
  OrionldContext*  contextP          = orionldContextCacheLookup(context);
  bool             crossNotification = (renderFormat >= RF_CROSS_APIS_NORMALIZED);           // RF_CROSS_APIS_[NORMALIZED|KEYVALUES|NORMALIZED_COMPACT|KEYVALUES_COMPACT]

  if (crossNotification == false)  // Meaning: NGSI-LD format
  {
    // id
    char notificationId[80];
    uuidGenerate(notificationId, sizeof(notificationId), "urn:ngsi-ld:Notification:");
    nodeP = kjString(orionldState.kjsonP, "id", notificationId);
    kjChildAdd(rootP, nodeP);

    // type
    nodeP = kjString(orionldState.kjsonP, "type", "Notification");
    kjChildAdd(rootP, nodeP);
  }

  // subscriptionId
  nodeP = kjString(orionldState.kjsonP, "subscriptionId", (char*) ncrP->subscriptionId.get().c_str());
  kjChildAdd(rootP, nodeP);

  // context - if JSONLD
  if ((mimeType == MT_JSONLD) && (crossNotification == false))
  {
    if (context == NULL)
      nodeP = kjString(orionldState.kjsonP, "@context", coreContextUrl);
    else
      nodeP = kjString(orionldState.kjsonP, "@context", context);

    kjChildAdd(rootP, nodeP);
  }
  // else - Context for HTTP Link Header is done in Notifier::buildSenderParams (Notifier.cpp)

  // notifiedAt
  if (crossNotification == false)  // Meaning: NGSI-LD format
  {
    char date[128];

    if (numberToDate(orionldState.requestTime, date, sizeof(date)) == false)
    {
      LM_E(("Runtime Error (numberToDate failed)"));
      return NULL;
    }
    nodeP = kjString(orionldState.kjsonP, "notifiedAt", date);
    kjChildAdd(rootP, nodeP);
  }

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
    if ((renderFormat != RF_CROSS_APIS_NORMALIZED) && (renderFormat != RF_CROSS_APIS_SIMPLIFIED))
    {
      alias = orionldContextItemAliasLookup(contextP, ceP->entityId.type.c_str(), NULL, NULL);
      nodeP = kjString(orionldState.kjsonP, "type", alias);
    }
    else
      nodeP = kjString(orionldState.kjsonP, "type", ceP->entityId.type.c_str());

    kjChildAdd(objectP, nodeP);

    // Attributes
    for (unsigned int aIx = 0; aIx < ceP->contextAttributeVector.size(); aIx++)
    {
      ContextAttribute*  aP       = ceP->contextAttributeVector[aIx];
      const char*        attrName = aP->name.c_str();

      if (strcmp(attrName, "@context") == 0)
        continue;

      nodeP = kjTreeFromContextAttribute(aP, contextP, renderFormat, lang, detailsP);
      if (nodeP == NULL)
      {
        LM_W(("kjTreeFromContextAttribute returned NULL"));
        continue;
      }

      kjChildAdd(objectP, nodeP);
    }

    //
    // Concise - transform/simplify the normalized format
    //
    if (renderFormat == RF_CONCISE)
      kjEntityNormalizedToConcise(objectP, NULL);
  }

  return rootP;
}
