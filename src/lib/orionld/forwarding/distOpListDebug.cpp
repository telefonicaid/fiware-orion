/*
*
* Copyright 2023 FIWARE Foundation e.V.
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
#include "logMsg/traceLevels.h"                                  // Lmt*

#include "orionld/forwarding/DistOp.h"                           // DistOp
#include "orionld/forwarding/distOpListDebug.h"                  // Own interface



// -----------------------------------------------------------------------------
//
// distOpListDebug -
//
void distOpListDebug(DistOp* distOpList, const char* what, const char* prefix)
{
  LM(("%s: Matching registrations (%s):", prefix, what));

  if (distOpList == NULL)
    LM(("%s:   None", prefix));

  int ix = 0;
  for (DistOp* distOpP = distOpList; distOpP != NULL; distOpP = distOpP->next)
  {
    LM(("%s:   DistOp %d: Reg Id: %s", prefix, ix, distOpP->regP->regId));
    ++ix;
  }
}



// -----------------------------------------------------------------------------
//
// distOpListDebug2 -
//
void distOpListDebug2(DistOp* distOpP, const char* what, const char* prefix)
{
  LM(("%s: ----- DistOp List: %s", what, prefix));

  if (distOpP == NULL)
    LM(("%s:   None", prefix));

  while (distOpP != NULL)
  {
    LM(("%s:   Registration:      %s", prefix, distOpP->regP->regId));
    LM(("%s:   Operation:         %s", prefix, distOpTypes[distOpP->operation]));

    if (distOpP->error == true)
    {
      LM(("%s:   Title:             %s", prefix, distOpP->title));
      LM(("%s:   Detail:            %s", prefix, distOpP->detail));
      LM(("%s:   Status:            %d", prefix, distOpP->httpResponseCode));
    }

    if (distOpP->requestBody != NULL)
    {
      LM(("%s:   Attributes:", prefix));
      int ix = 0;
      for (KjNode* attrP = distOpP->requestBody->value.firstChildP; attrP != NULL; attrP = attrP->next)
      {
        if ((strcmp(attrP->name, "id") != 0) && (strcmp(attrP->name, "type") != 0))
        {
          LM(("%s:     Attribute %d:   '%s'", prefix, ix, attrP->name));
          ++ix;
        }
      }
    }

    if (distOpP->attrList != NULL)
    {
      LM(("%s:   URL Attributes:        %d", prefix, distOpP->attrList->items));
      for (int ix = 0; ix < distOpP->attrList->items; ix++)
      {
        LM(("%s:     Attribute %d:   '%s'", prefix, ix, distOpP->attrList->array[ix]));
      }
    }

    if (distOpP->typeList != NULL)
    {
      LM(("%s:   URL Entity Types:        %d", prefix, distOpP->typeList->items));
      for (int ix = 0; ix < distOpP->typeList->items; ix++)
      {
        LM(("%s:     Entity Type %02d:   '%s'", prefix, ix, distOpP->typeList->array[ix]));
      }
    }

    if (distOpP->idList != NULL)
    {
      LM(("%s:   URL Entity IDs:        %d", prefix, distOpP->idList->items));
      for (int ix = 0; ix < distOpP->idList->items; ix++)
      {
        LM(("%s:     Entity ID %02d:   '%s'", prefix, ix, distOpP->idList->array[ix]));
      }
    }

    if (distOpP->entityId != NULL)
      LM(("%s:   URL Entity ID:         %s", prefix, distOpP->entityId));
    if (distOpP->entityIdPattern != NULL)
      LM(("%s:   URL Entity ID Pattern: %s", prefix, distOpP->entityIdPattern));
    if (distOpP->entityType != NULL)
      LM(("%s:   URL Entity TYPE:       %s", prefix, distOpP->entityType));

    LM(("%s: ----------------------------------------", prefix));

    distOpP = distOpP->next;
  }

  LM(("%s: ---------------------", prefix));
}
