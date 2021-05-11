/*
*
* Copyright 2019 FIWARE Foundation e.V.
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
}

#include "logMsg/logMsg.h"                                       // LM_*
#include "logMsg/traceLevels.h"                                  // Lmt*

#include "orionld/types/OrionldProblemDetails.h"                 // OrionldProblemDetails, orionldProblemDetailsFill
#include "orionld/common/orionldState.h"                         // orionldState, kalloc
#include "orionld/contextCache/orionldContextCacheInsert.h"      // orionldContextCacheInsert
#include "orionld/contextCache/orionldContextCacheLookup.h"      // orionldContextCacheLookup
#include "orionld/contextCache/orionldContextCachePresent.h"     // orionldContextCachePresent
#include "orionld/context/OrionldContext.h"                      // OrionldContext
#include "orionld/context/orionldContextCreate.h"                // orionldContextCreate
#include "orionld/context/orionldContextUrlGenerate.h"           // orionldContextUrlGenerate
#include "orionld/context/orionldContextFromUrl.h"               // orionldContextFromUrl
#include "orionld/context/orionldContextFromObject.h"            // orionldContextFromObject
#include "orionld/context/orionldContextFromArray.h"             // Own interface



// -----------------------------------------------------------------------------
//
// orionldContextFromArray -
//
OrionldContext* orionldContextFromArray(char* url, OrionldContextOrigin origin, bool toBeCloned, int itemsInArray, KjNode* contextArrayP, OrionldProblemDetails* pdP)
{
  char*           id = NULL;
  OrionldContext* contextP;

  if (url == NULL)
    url = orionldContextUrlGenerate(&id);

  contextP = orionldContextCreate(url, origin, id, contextArrayP, false, toBeCloned);
  orionldContextCacheInsert(contextP);

  contextP->context.array.items    = itemsInArray;
  contextP->context.array.vector   = (OrionldContext**) kaAlloc(&kalloc, itemsInArray * sizeof(OrionldContext*));


  //
  // Valid types for members of a context array are:   KjString and KjObject
  // I could probably easily allow KjArray also.
  //
  // If KjString, the url must be looked up first, then orionldContextFromUrl is called
  // If KjObject, the object is made a "standalone" vontext and replaced in the KjNode tree by a string (its URL)
  //
  int slot = itemsInArray - 1;  // NOTE: Insertion starts at the end of the array - the array is sorted backwards

  for (KjNode* arrayItemP = contextArrayP->value.firstChildP; arrayItemP != NULL; arrayItemP = arrayItemP->next)
  {
    OrionldContext* childContextP;

    if (arrayItemP->type == KjString)
    {
      childContextP = orionldContextCacheLookup(arrayItemP->value.s);
      if (childContextP == NULL)
        childContextP = orionldContextFromUrl(arrayItemP->value.s, NULL, pdP);
      if (childContextP == NULL)
      {
        // orionldContextFromUrl fills in pdP
        LM_E(("orionldContextFromUrl: %s: %s", pdP->title, pdP->detail));
        return NULL;
      }
    }
    else if (arrayItemP->type == KjObject)
    {
      childContextP = orionldContextFromObject(NULL, origin, NULL, false, arrayItemP, pdP);

      if (childContextP == NULL)
      {
        // orionldContextFromObject fills in pdP
        LM_E(("CTX: orionldContextFromObject: %s: %s", pdP->title, pdP->detail));
        return NULL;
      }
    }
    else
    {
      LM_E(("invalid type of @context array item: %s", kjValueType(arrayItemP->type)));
      pdP->type   = OrionldBadRequestData;
      pdP->title  = (char*) "Invalid @context - invalid type for @context array item";
      pdP->detail = (char*) kjValueType(arrayItemP->type);
      pdP->status = 400;

      return NULL;
    }

    contextP->context.array.vector[slot] = childContextP;
    --slot;
  }

  return contextP;
}
