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
#include <string.h>                                               // strcmp

extern "C"
{
#include "kjson/KjNode.h"                                         // KjNode
}

#include "orionld/kjTree/kjStringArraySortedInsert.h"             // Own interface



// -----------------------------------------------------------------------------
//
// kjStringArraySortedInsert -
//
void kjStringArraySortedInsert(KjNode* array, KjNode* newItemP)
{
  KjNode* prev  = NULL;
  KjNode* itemP = array->value.firstChildP;

  while (itemP != NULL)
  {
    int cmp = strcmp(itemP->value.s, newItemP->value.s);  // <0 if itemP < newItemP,  ==0 id equal

    if (cmp < 0)
      prev = itemP;
    else if (cmp == 0)
      return;  // Already present values are skipped

    itemP = itemP->next;
  }

  if (prev != NULL)
  {
    if (prev->next == NULL)  // Insert as last item
    {
      prev->next       = newItemP;
      newItemP->next   = NULL;
      array->lastChild = newItemP;
    }
    else  // Insert the middle
    {
      newItemP->next = prev->next;
      prev->next     = newItemP;
    }
  }
  else  // Insert as first item
  {
    newItemP->next = array->value.firstChildP;
    array->value.firstChildP = newItemP;

    if (array->lastChild == NULL)
      array->lastChild = newItemP;
  }
}
