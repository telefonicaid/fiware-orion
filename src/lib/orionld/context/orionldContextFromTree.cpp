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
#include "orionld/context/OrionldContext.h"                      // OrionldContext
#include "orionld/context/orionldContextSimplify.h"              // orionldContextSimplify
#include "orionld/context/orionldContextFromUrl.h"               // orionldContextFromUrl
#include "orionld/context/orionldContextFromObject.h"            // orionldContextFromObject
#include "orionld/context/orionldContextFromArray.h"             // orionldContextFromArray
#include "orionld/context/orionldContextFromTree.h"              // Own interface


// -----------------------------------------------------------------------------
//
// orionldContextFromTree -
//
OrionldContext* orionldContextFromTree(char* url, bool toBeCloned, KjNode* contextTreeP, OrionldProblemDetails* pdP)
{
  int itemsInArray;

  if (contextTreeP->type == KjArray)
  {
    contextTreeP = orionldContextSimplify(contextTreeP, &itemsInArray);
    if (contextTreeP == NULL)
    {
      pdP->type   = OrionldBadRequestData;
      pdP->title  = (char*) "Empty @context";
      pdP->detail = (char*) "got an array with only Core Context";
      pdP->status = 200;

      return NULL;
    }
  }

  if      (contextTreeP->type == KjString)   return orionldContextFromUrl(contextTreeP->value.s, pdP);
  else if (contextTreeP->type == KjObject)   return orionldContextFromObject(url, toBeCloned, contextTreeP, pdP);
  else if (contextTreeP->type == KjArray)    return orionldContextFromArray(url, toBeCloned, itemsInArray, contextTreeP, pdP);

  //
  // None of the above. Error
  //
  pdP->type   = OrionldBadRequestData;
  pdP->title  = (char*) "Invalid type for item in @context array";
  pdP->detail = (char*) kjValueType(contextTreeP->type);
  pdP->status = 400;

  LM_E(("%s: %s", pdP->title, pdP->detail));
  return NULL;
}
