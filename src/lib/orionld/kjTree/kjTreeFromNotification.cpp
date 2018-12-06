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
#include "ngsi10/NotifyContextRequest.h"                       // NotifyContextRequest
#include "mongoBackend/MongoGlobal.h"                          // mongoIdentifier
#include "orionld/common/OrionldConnection.h"                  // orionldState
#include "orionld/common/numberToDate.h"                       // numberToDate
#include "orionld/kjTree/kjTreeFromContextAttribute.h"         // kjTreeFromContextAttribute
#include "orionld/kjTree/kjTreeFromNotification.h"             // Own interface



// -----------------------------------------------------------------------------
//
// kjTreeFromNotification -
//
KjNode* kjTreeFromNotification(NotifyContextRequest* ncrP, char** detailsP)
{
  KjNode* nodeP;
  char    buf[32];
  KjNode* root = kjObject(NULL, NULL);
  char*   id   = mongoIdentifier(buf);
  char    idBuffer[] = "urn:ngsi-ld:Notification:012345678901234567890123";  // The 012345678901234567890123 will be overwritten

  // id
  strcpy(&idBuffer[25], id);
  nodeP = kjString(orionldState.kjsonP, "id", idBuffer);
  kjChildAdd(root, nodeP);

  // type
  nodeP = kjString(orionldState.kjsonP, "type", "Notification");
  kjChildAdd(root, nodeP);

  // subscriptionId
  nodeP = kjString(orionldState.kjsonP, "subscriptionId", (char*) ncrP->subscriptionId.get().c_str());
  kjChildAdd(root, nodeP);

  // notifiedAt
  time_t  now = time(NULL);  // FIXME - use an already existing timestamp?
  char    date[25];
  char*   details;

  if (numberToDate(now, date, sizeof(date), &details) == false)
  {
    LM_E(("Runtime Error (numberToDate: %s)", details));
    return NULL;
  }

  // data
  KjNode* dataP = kjArray(orionldState.kjsonP, "data");
  kjChildAdd(root, dataP);
  
  //
  // loop over ContextElements in NotifyContextRequest::contextElementResponseVector
  //
  LM_TMP(("KZ: Adding %d contextElementResponses to the Notification kjTree", (int) ncrP->contextElementResponseVector.size()));
  for (unsigned int ix; ix < ncrP->contextElementResponseVector.size(); ix++)
  {
    ContextElement* ceP     = &ncrP->contextElementResponseVector[ix]->contextElement;
    KjNode*         objectP = kjObject(orionldState.kjsonP, NULL);
    KjNode*         nodeP;

    kjChildAdd(dataP, objectP);

    // entity id - Mandatory URI
    nodeP = kjString(orionldState.kjsonP, "id", ceP->entityId.id.c_str());
    kjChildAdd(objectP, nodeP);
    
    // entity type - Mandatory URI
    nodeP = kjString(orionldState.kjsonP, "type", ceP->entityId.type.c_str());
    kjChildAdd(objectP, nodeP);

    LM_TMP(("KZ: Adding %d attributes to the Notification kjTree", (int) ceP->contextAttributeVector.size()));
    for (unsigned int aIx = 0; aIx < ceP->contextAttributeVector.size(); aIx++)
    {
      LM_TMP(("KZ: Adding attribute '%s' to the Notification kjTree", ceP->contextAttributeVector[aIx]->name.c_str()));
      nodeP = kjTreeFromContextAttribute(ceP->contextAttributeVector[aIx], detailsP);
      kjChildAdd(objectP, nodeP);
    }
    // location                        GeoProperty
    // observationSpace                GeoProperty
    // operationSpace                  GeoProperty
    // Property 0..N
    // Relationship 0..N
    //

  }
  
  return root;
}
