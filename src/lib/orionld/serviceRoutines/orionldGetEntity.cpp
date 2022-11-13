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
extern "C"
{
#include "kjson/KjNode.h"                                        // KjNode
#include "kjson/kjLookup.h"                                      // kjLookup
#include "kjson/kjBuilder.h"                                     // kjChildRemove, kjChildAdd, kjArray, ...
}

#include "logMsg/logMsg.h"                                       // LM_*

#include "orionld/common/orionldState.h"                         // orionldState
#include "orionld/common/orionldError.h"                         // orionldError
#include "orionld/payloadCheck/pCheckUri.h"                      // pCheckUri
#include "orionld/legacyDriver/legacyGetEntity.h"                // legacyGetEntity
#include "orionld/mongoc/mongocEntityLookup.h"                   // mongocEntityLookup
#include "orionld/dbModel/dbModelToApiEntity.h"                  // dbModelToApiEntity
#include "orionld/kjTree/kjGeojsonEntityTransform.h"             // kjGeojsonEntityTransform
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
  if ((experimental == false) || (orionldState.in.legacy != NULL))                      // If Legacy header - use old implementation
    return legacyGetEntity();

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
    // BUT, distributes hasn't been implemented yet, so ...
    const char* title = (orionldState.in.attrList.items != 0)? "Combination Entity/Attributes Not Found" : "Entity Not Found";
    orionldError(OrionldResourceNotFound, title, entityId, 404);
    return false;
  }

  KjNode* apiEntityP = dbModelToApiEntity2(dbEntityP, orionldState.uriParamOptions.sysAttrs, orionldState.out.format, orionldState.uriParams.lang, true, &orionldState.pd);
  orionldState.responseTree   = apiEntityP;
  orionldState.httpStatusCode = 200;

  if (orionldState.out.contentType == GEOJSON)
  {
    orionldState.responseTree = kjGeojsonEntityTransform(orionldState.responseTree, orionldState.geoPropertyNode);

    //
    // If URI params 'attrs' and 'geometryProperty' are given BUT 'geometryProperty' is not part of 'attrs', then we need to remove 'geometryProperty' from
    // the "properties" object
    //
    if ((orionldState.in.attrList.items > 0) && (orionldState.uriParams.geometryProperty != NULL))
    {
      bool geometryPropertyInAttrList = false;

      for (int ix = 0; ix < orionldState.in.attrList.items; ix++)
      {
        if (strcmp(orionldState.in.geometryPropertyExpanded, orionldState.in.attrList.array[ix]) == 0)
        {
          geometryPropertyInAttrList = true;
          break;
        }
      }

      if (geometryPropertyInAttrList == false)
      {
        KjNode* propertiesP = kjLookup(orionldState.responseTree, "properties");

        if (propertiesP != NULL)
        {
          KjNode* geometryPropertyP = kjLookup(propertiesP, orionldState.in.geometryPropertyExpanded);

          if (geometryPropertyP != NULL)
            kjChildRemove(propertiesP, geometryPropertyP);
        }
      }
    }
  }

  return true;
}
