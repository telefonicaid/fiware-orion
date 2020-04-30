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
#include "kalloc/kaStrdup.h"                                     // kaStrdup
}

#include "logMsg/logMsg.h"                                       // LM_*
#include "logMsg/traceLevels.h"                                  // Lmt*

#include "rest/ConnectionInfo.h"                                 // ConnectionInfo

#include "orionld/common/orionldState.h"                         // orionldState
#include "orionld/common/eqForDot.h"                             // eqForDot
#include "orionld/context/orionldContextItemAliasLookup.h"       // orionldContextItemAliasLookup
#include "orionld/serviceRoutines/orionldGetDbIndexes.h"         // Own interface



// ----------------------------------------------------------------------------
//
// orionldGetDbIndexes -
//
bool orionldGetDbIndexes(ConnectionInfo* ciP)
{
  int items = 0;

  orionldState.responseTree = kjArray(orionldState.kjsonP, NULL);

  for (OrionldGeoIndex* geoNodeP = geoIndexList; geoNodeP != NULL; geoNodeP = geoNodeP->next)
  {
    char*   attrName  =  kaStrdup(&orionldState.kalloc, geoNodeP->attrName);

    eqForDot(attrName);

    KjNode* objP      = kjObject(orionldState.kjsonP, NULL);
    KjNode* tenantP   = kjString(orionldState.kjsonP, "tenant",   geoNodeP->tenant);
    KjNode* attrNameP = kjString(orionldState.kjsonP, "attribute", orionldContextItemAliasLookup(orionldState.contextP, attrName, NULL, NULL));

    kjChildAdd(objP, tenantP);
    kjChildAdd(objP, attrNameP);
    kjChildAdd(orionldState.responseTree, objP);

    ++items;
  }

  if (items == 0)
    orionldState.noLinkHeader = true;

  return true;
}
