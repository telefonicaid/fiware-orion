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
#include "logMsg/logMsg.h"                                       // LM_*

#include "orionld/common/orionldState.h"                         // orionldState
#include "orionld/common/orionldError.h"                         // orionldError
#include "orionld/payloadCheck/pCheckUri.h"                      // pCheckUri
#include "orionld/legacyDriver/legacyGetEntity.h"                // legacyGetEntity
#include "orionld/mongoc/mongocEntityLookup.h"                   // mongocEntityLookup
#include "orionld/serviceRoutines/orionldGetEntity.h"            // Own interface



// ----------------------------------------------------------------------------
//
// orionldGetEntity -
//
// URL PARAMETERS
// - attrs
// - geometryProperty
// - lang
// - options=simplified/concise  (normalized is default)
//
bool orionldGetEntity(void)
{
  // if ((experimental == false) || (orionldState.in.legacy != NULL))                      // If Legacy header - use old implementation
    return legacyGetEntity();
#if 0
  const char* entityId = orionldState.wildcard[0];
  if (pCheckUri(entityId, "Entity ID in URL PATH", true) == false)
    return false;

  // <DEBUG>
  LM(("IN: EntityId: %s", entityId));
  for (int ix = 0; ix < orionldState.in.attrList.items; ix++)
  {
    LM(("IN: Attr %d:   %s", ix, orionldState.in.attrList.array[ix]));
  }

  if (orionldState.uriParams.geometryProperty != NULL)
    LM(("IN: GeoProp:  %s", orionldState.uriParams.geometryProperty));
  
  if (orionldState.uriParamOptions.keyValues == true)
    LM(("IN: InFormat: Simplified"));
  else if (orionldState.uriParamOptions.concise == true)
    LM(("IN: InFormat: Concise"));
  else
    LM(("IN: InFormat: Normalized"));
    
  LM(("IN: DataMode:  %s", (orionldState.uriParams.local == true)? "LocalOnly" : "Distributed"));
  // <DEBUG>

  bool distributed = (forwarding == true) || (orionldState.uriParams.local == false);
  if (distributed)
  {
    // If 'attrs' is used, any matches in Exclusive registrations must chop off attributes from 'attrs'
  }

  // Now GET the local data (if any)
  KjNode* dbEntityP = mongocEntityLookup(entityId, &orionldState.in.attrList, orionldState.uriParams.geometryProperty);
  if (dbEntityP == NULL)
  {
    if (distributed == false)
    {
      const char* title = (orionldState.in.attrList.items != 0)? "Combination Entity/Attributes Not Found" : "Entity Not Found";
      orionldError(OrionldResourceNotFound, title, entityId, 404);
      return false;
    }
    // If distributed, then it's perfectly OK to have nothing locally
  }

  return false;
#endif
}
