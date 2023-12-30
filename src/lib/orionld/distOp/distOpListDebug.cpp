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
#include "logMsg/logMsg.h"                                       // LM_T, lmTraceIsSet
#include "logMsg/traceLevels.h"                                  // distOpListDebug2

#include "orionld/types/DistOp.h"                                // DistOp
#include "orionld/distOp/distOpListDebug.h"                      // Own interface



// -----------------------------------------------------------------------------
//
// distOpListDebug -
//
void distOpListDebug(DistOp* distOpList, const char* what)
{
  if (lmTraceIsSet(LmtDistOpList) == false)
    return;

  LM_T(LmtDistOpList, ("Matching registrations (%s):", what));

  if (distOpList == NULL)
    LM_T(LmtDistOpList, ("   None"));

  int ix = 0;
  for (DistOp* distOpP = distOpList; distOpP != NULL; distOpP = distOpP->next)
  {
    LM_T(LmtDistOpList, ("   DistOp %d: Reg Id: %s", ix, distOpP->regP->regId));
    ++ix;
  }
}



// -----------------------------------------------------------------------------
//
// distOpListDebug2 -
//
void distOpListDebug2(DistOp* distOpP, const char* what)
{
  if (lmTraceIsSet(LmtDistOpList) == false)
    return;

  LM_T(LmtDistOpList, ("----- DistOp List: %s", what));

  if (distOpP == NULL)
    LM_T(LmtDistOpList, ("  None"));

  while (distOpP != NULL)
  {
    LM_T(LmtDistOpList, ("  DistOp ID:         %s", distOpP->id));
    LM_T(LmtDistOpList, ("  Registration:      %s", (distOpP->regP != NULL)? distOpP->regP->regId : "local DB"));
    LM_T(LmtDistOpList, ("  Operation:         %s", distOpTypes[distOpP->operation]));

    if (distOpP->error == true)
    {
      LM_T(LmtDistOpList, ("  Error:"));
      LM_T(LmtDistOpList, ("    Title:             %s", distOpP->title));
      LM_T(LmtDistOpList, ("    Detail:            %s", distOpP->detail));
      LM_T(LmtDistOpList, ("    Status:            %d", distOpP->httpResponseCode));
    }

    if (distOpP->requestBody != NULL)
    {
      if (distOpP->operation == DoDeleteBatch)
      {
        LM_T(LmtDistOpList, ("  Entity IDs:"));
        for (KjNode* eIdNodeP = distOpP->requestBody->value.firstChildP; eIdNodeP != NULL; eIdNodeP = eIdNodeP->next)
        {
          LM_T(LmtDistOpList, ("  o %s", eIdNodeP->value.s));
        }
      }
      else
      {
        LM_T(LmtDistOpList, ("  Attributes:"));

        int ix = 0;
        for (KjNode* attrP = distOpP->requestBody->value.firstChildP; attrP != NULL; attrP = attrP->next)
        {
          if ((strcmp(attrP->name, "id") != 0) && (strcmp(attrP->name, "type") != 0))
          {
            LM_T(LmtDistOpList, ("    Attribute %d:   '%s'", ix, attrP->name));
            ++ix;
          }
        }
      }
    }

    if (distOpP->attrList != NULL)
    {
      LM_T(LmtDistOpList, ("  URL Attributes:        %d", distOpP->attrList->items));
      for (int ix = 0; ix < distOpP->attrList->items; ix++)
      {
        LM_T(LmtDistOpList, ("    Attribute %d:   '%s'", ix, distOpP->attrList->array[ix]));
      }
    }

    if (distOpP->attrsParam != NULL)
    {
      LM_T(LmtDistOpList, ("  URL Attributes:        '%s' (len: %d)", distOpP->attrsParam, distOpP->attrsParamLen));
    }

    if (distOpP->typeList != NULL)
    {
      LM_T(LmtDistOpList, ("  URL Entity Types:        %d", distOpP->typeList->items));
      for (int ix = 0; ix < distOpP->typeList->items; ix++)
      {
        LM_T(LmtDistOpList, ("    Entity Type %02d:   '%s'", ix, distOpP->typeList->array[ix]));
      }
    }

    if (distOpP->idList != NULL)
    {
      LM_T(LmtDistOpList, ("  URL Entity IDs:        %d", distOpP->idList->items));
      for (int ix = 0; ix < distOpP->idList->items; ix++)
      {
        LM_T(LmtDistOpList, ("    Entity ID %02d:   '%s'", ix, distOpP->idList->array[ix]));
      }
    }

    if (distOpP->entityId != NULL)
      LM_T(LmtDistOpList, ("  URL Entity ID:         %s", distOpP->entityId));
    if (distOpP->entityIdPattern != NULL)
      LM_T(LmtDistOpList, ("  URL Entity ID Pattern: %s", distOpP->entityIdPattern));
    if (distOpP->entityType != NULL)
      LM_T(LmtDistOpList, ("  URL Entity TYPE:       %s", distOpP->entityType));

    LM_T(LmtDistOpList, ("----------------------------------------"));

    distOpP = distOpP->next;
  }

  LM_T(LmtDistOpList, ("---------------------"));
}
