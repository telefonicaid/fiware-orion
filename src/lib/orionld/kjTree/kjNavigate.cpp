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
#include <unistd.h>                                              // NULL

extern "C"
{
#include "kjson/KjNode.h"                                        // KjNode
#include "kjson/kjLookup.h"                                      // kjLookup
}

#include "orionld/kjTree/kjNavigate.h"                           // Own interface



// -----------------------------------------------------------------------------
//
// kjNavigate -
//
// FIXME: move to kjson library
//
KjNode* kjNavigate(KjNode* treeP, char** pathCompV, KjNode** parentPP, bool* onlyLastMissingP)
{
  KjNode* hitP = kjLookup(treeP, pathCompV[0]);

  *parentPP = treeP;

  if (hitP == NULL)  // No hit - we're done
  {
    *onlyLastMissingP = (pathCompV[1] == NULL)? true : false;
    return NULL;
  }

  if (pathCompV[1] == NULL)  // Found it - we're done
    return hitP;

  return kjNavigate(hitP, &pathCompV[1], parentPP, onlyLastMissingP);  // Recursive call, one level down
}
