/*
*
* Copyright 2021 FIWARE Foundation e.V.
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
#include <string.h>                                              // strcmp

extern "C"
{
#include "kjson/KjNode.h"                                        // KjNode
#include "kjson/kjBuilder.h"                                     // kjChildAdd
#include "kjson/kjRender.h"                                      // kjFastRender
}

#include "logMsg/logMsg.h"                                       // LM_*
#include "logMsg/traceLevels.h"                                  // Lmt*

#include "orionld/common/orionldState.h"                         // orionldState
#include "orionld/kjTree/kjSort.h"                               // Own interface



// -----------------------------------------------------------------------------
//
// kjSort - sort a KjNode Object
//
// Objects are sorted alphabetically by key.
// Arrays cannot be sorted.
//
void kjSort(KjNode* nodeP)
{
  if ((nodeP->type != KjObject) && (nodeP->type != KjArray))  // Arrays can contain objects, that can be sorted,
    return;

  if (nodeP->value.firstChildP == NULL)
    return;

  // Recursive calls for all child items that are Object or Array
  for (KjNode* currentP = nodeP->value.firstChildP; currentP != NULL; currentP = currentP->next)
  {
    if ((currentP->type == KjObject) || (currentP->type == KjArray))
      kjSort(currentP);
  }

  // Arrays aren't sorted
  if (nodeP->type == KjArray)
    return;

  KjNode* startP = nodeP->value.firstChildP;
  KjNode* maxP   = startP;

  while (1)
  {
    // Find MAX node
    maxP = startP;
    for (KjNode* currentP = startP->next; currentP != NULL; currentP = currentP->next)
    {
      if (strcmp(maxP->name, currentP->name) > 0)
        maxP = currentP;
    }

    // Put maxP at the beginning
    KjNode* next = startP->next;
    if (maxP != nodeP->value.firstChildP)
    {
      kjChildRemove(nodeP, maxP);
      maxP->next = nodeP->value.firstChildP;
      nodeP->value.firstChildP = maxP;
    }

    if (maxP == startP)
      startP = next;

    if ((startP == NULL) || (startP->next == NULL))
      break;
  }
}



// -----------------------------------------------------------------------------
//
// kjStringArraySort - sort a KjNode Array
//
// Elements (KjString) are sorted alphabetically by value.
//
void kjStringArraySort(KjNode* arrayP)
{
  KjNode* rawP = kjArray(orionldState.kjsonP, NULL);

  //
  // Put all items in rawP, leaving arrayP empty
  //
  rawP->value.firstChildP   = arrayP->value.firstChildP;
  rawP->lastChild           = arrayP->lastChild;
  arrayP->value.firstChildP = NULL;
  arrayP->lastChild         = NULL;

  //
  // Loop over rawP, find smallest, then just move it to arrayP
  //
  while (rawP->value.firstChildP != NULL)
  {
    // Set the very first item as the smallest one (minP)
    KjNode* minP = rawP->value.firstChildP;

    // Compare with all the rest and set minP accordingly
    for (KjNode* tmpP = minP->next; tmpP != NULL; tmpP = tmpP->next)
    {
      if (strcmp(minP->value.s, tmpP->value.s) > 0)
        minP = tmpP;
    }

    // Move the smallest from rawP to arrayP
    kjChildRemove(rawP, minP);
    kjChildAdd(arrayP, minP);
  }
}
