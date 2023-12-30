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
#include "orionld/types/DistOp.h"                                // DistOp
#include "orionld/distOp/distOpListsMerge.h"                     // Own interface



// ----------------------------------------------------------------------------
//
// distOpListsMerge -
//
DistOp* distOpListsMerge(DistOp* list1, DistOp* list2)
{
  if (list2 == NULL)
    return list1;
  else if (list1 == NULL)
    return list2;

  //
  // Both lists are non-NULL
  // Find the last item in list1 and make its next pointer point to the first in list2
  //
  DistOp* lastInList1 = list1;
  while (lastInList1->next != NULL)
    lastInList1 = lastInList1->next;

  lastInList1->next = list2;

  return list1;
}
