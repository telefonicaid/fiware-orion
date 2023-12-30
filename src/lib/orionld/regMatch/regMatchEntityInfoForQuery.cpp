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
#include <string.h>                                              // strcmp

extern "C"
{
#include "kjson/KjNode.h"                                        // KjNode
#include "kjson/kjLookup.h"                                      // kjLookup
}

#include "logMsg/logMsg.h"                                       // LM_T

#include "orionld/types/RegCacheItem.h"                          // RegCacheItem, RegIdPattern
#include "orionld/types/StringArray.h"                           // StringArray
#include "orionld/types/DistOp.h"                                // DistOp
#include "orionld/regMatch/regMatchEntityInfoForQuery.h"         // regMatchEntityInfoForQuery



// -----------------------------------------------------------------------------
//
// regIdPatternLookup - from regMatchEntityInfo.cpp
//
extern RegIdPattern* regIdPatternLookup(RegCacheItem* regP, KjNode* idPatternP);



// -----------------------------------------------------------------------------
//
// regMatchEntityInfoForQuery -
//
// Parameters:
//   regP:        Pointer to the registration in the reg-cache
//   entityInfoP: { "type": "T", "id": "urn:E1" }
//   idListP:     StringArray of Entity Ids (can be empty)
//   typeListP:   StringArray of Entity Types (can be empty)
//
bool regMatchEntityInfoForQuery(RegCacheItem* regP, KjNode* entityInfoP, StringArray* idListP, StringArray* typeListP, DistOp* distOpP)
{
  KjNode* regEntityTypeP       = kjLookup(entityInfoP, "type");
  KjNode* regEntityIdP         = kjLookup(entityInfoP, "id");
  KjNode* regEntityIdPatternP  = kjLookup(entityInfoP, "idPattern");

  // This should never happen
  if (regEntityTypeP == NULL)
  {
    LM_E(("%s: invalid registration (no type in information::entities)"));
    return false;
  }


  //
  // If the query had any entity types present we need to match the entityInfoP against that
  //
  if (typeListP->items > 0)
  {
    char* entityType = regEntityTypeP->value.s;
    bool  match      = false;

    for (int ix = 0; ix < typeListP->items; ix++)
    {
      if (strcmp(typeListP->array[ix], entityType) == 0)
      {
        match = true;
        break;
      }
    }

    if (match == false)
    {
      LM_T(LmtRegMatch, ("%s: No Reg Match due to entity type ('%s' in reg)", regP->regId, entityType));
      return false;
    }
  }

  //
  // If the query had any entity ids present we need to match the entityInfoP against that
  // This can be either matched against the id in the regiostration OR the idPattern
  //
  if (idListP->items > 0)
  {
    if (regEntityIdP != NULL)
    {
      char* regEntityId = regEntityIdP->value.s;
      bool  match       = false;

      for (int ix = 0; ix < idListP->items; ix++)
      {
        if (strcmp(idListP->array[ix], regEntityId) == 0)
        {
          match = true;
          break;
        }
      }

      if (match == false)
      {
        LM_T(LmtRegMatch, ("%s: No Reg Match due to entity id ('%s' in reg)", regP->regId, regEntityId));
        return false;
      }
    }
    else if (regEntityIdPatternP != NULL)
    {
      RegIdPattern* ripP = regIdPatternLookup(regP, regEntityIdPatternP);

      if (ripP == NULL)
      {
        LM_E(("%s: Internal Error (the regex corresponding to this idPattern could not be found)", regP->regId));
        return false;
      }
      else
      {
        bool  match = false;

        for (int ix = 0; ix < idListP->items; ix++)
        {
          char* entityId = idListP->array[ix];

          if (regexec(&ripP->regex, entityId, 0, NULL, 0) != 0)
          {
            match = true;
            break;
          }
        }

        if (match == false)
        {
          LM_T(LmtRegMatch, ("%s: No Reg Match due to entity id (idPattern in registration VS query entity-id-list)", regP->regId));
          return false;
        }
      }
    }
  }

  if (regEntityIdP != NULL)
  {
    distOpP->entityId = regEntityIdP->value.s;
    distOpP->idList   = NULL;
  }
  else
    distOpP->idList = idListP;

  if ((regEntityIdP == NULL) && (regEntityIdPatternP != NULL))
  {
    distOpP->entityIdPattern = regEntityIdPatternP->value.s;
    distOpP->idList          = NULL;
  }

  if (regEntityTypeP != NULL)  // "type" is mandatory. regEntityTypeP cannot be NULL
  {
    distOpP->entityType = regEntityTypeP->value.s;
    distOpP->typeList   = NULL;
  }
  else
    distOpP->typeList = typeListP;

  return true;
}
