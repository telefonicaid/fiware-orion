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
void distOpListDebug(DistOp* distOpList, const char* what)
{
  LM(("Matching registrations (%s):", what));
  int ix = 0;
  for (DistOp* distOpP = distOpList; distOpP != NULL; distOpP = distOpP->next)
  {
    LM(("  DistOp %d: Reg Id: %s", ix, distOpP->regP->regId));
    ++ix;
  }
}



// -----------------------------------------------------------------------------
//
// distOpListDebug2 -
//
void distOpListDebug2(DistOp* distOpP, const char* what)
{
  LM(("----- DistOp List: %s", what));

  while (distOpP != NULL)
  {
    LM(("  Registration:      %s", distOpP->regP->regId));
    LM(("  Operation:         %s", distOpTypes[distOpP->operation]));

    if (distOpP->error == true)
    {
      LM(("  Title:             %s", distOpP->title));
      LM(("  Detail:            %s", distOpP->detail));
      LM(("  Status:            %d", distOpP->httpResponseCode));
    }

    if (distOpP->requestBody != NULL)
    {
      LM(("  Attributes:"));
      int ix = 0;
      for (KjNode* attrP = distOpP->requestBody->value.firstChildP; attrP != NULL; attrP = attrP->next)
      {
        if ((strcmp(attrP->name, "id") != 0) && (strcmp(attrP->name, "type") != 0))
        {
          LM(("    Attribute %d:   '%s'", ix, attrP->name));
          ++ix;
        }
      }
    }

    if (distOpP->attrList != NULL)
    {
      LM(("  URL Attributes:        %d", distOpP->attrList->items));
      for (int ix = 0; ix < distOpP->attrList->items; ix++)
      {
        LM(("    Attribute %d:   '%s'", ix, distOpP->attrList->array[ix]));
      }
    }

    if (distOpP->typeList != NULL)
    {
      LM(("  URL Entity Types:        %d", distOpP->typeList->items));
      for (int ix = 0; ix < distOpP->typeList->items; ix++)
      {
        LM(("    Entity Type %02d:   '%s'", ix, distOpP->typeList->array[ix]));
      }
    }

    if (distOpP->idList != NULL)
    {
      LM(("  URL Entity IDs:        %d", distOpP->idList->items));
      for (int ix = 0; ix < distOpP->idList->items; ix++)
      {
        LM(("    Entity ID %02d:   '%s'", ix, distOpP->idList->array[ix]));
      }
    }

    if (distOpP->entityId != NULL)
      LM(("  URL Entity ID:         %s", distOpP->entityId));
    if (distOpP->entityIdPattern != NULL)
      LM(("  URL Entity ID Pattern: %s", distOpP->entityIdPattern));
    if (distOpP->entityType != NULL)
      LM(("  URL Entity TYPE:       %s", distOpP->entityType));

    LM(("----------------------------------------"));

    distOpP = distOpP->next;
  }

  LM(("---------------------"));
}
