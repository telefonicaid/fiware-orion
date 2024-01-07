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
#include <vector>

#include "logMsg/logMsg.h"                                     // LM_*
#include "logMsg/traceLevels.h"                                // Lmt*

extern "C"
{
#include "kjson/KjNode.h"                                      // KjNode
}

#include "apiTypesV2/EntID.h"                                    // EntID

#include "orionld/common/orionldState.h"                         // orionldState
#include "orionld/common/orionldError.h"                         // orionldError
#include "orionld/common/SCOMPARE.h"                             // SCOMPAREx
#include "orionld/common/CHECK.h"                                // CHECKx(U)
#include "orionld/payloadCheck/PCHECK.h"                         // PCHECK_
#include "orionld/context/orionldContextItemExpand.h"            // orionldContextItemExpand
#include "orionld/legacyDriver/kjTreeToEntIdVector.h"            // Own interface



// -----------------------------------------------------------------------------
//
// kjTreeToEntIdVector -
//
// ngsiv2::EntID is used, but the type is really EntityInfo, and it has three fields:
// - id
// - idPattern
// - type
//
bool kjTreeToEntIdVector(KjNode* kNodeP, std::vector<ngsiv2::EntID>* entitiesP)
{
  KjNode* entityP;

  for (entityP = kNodeP->value.firstChildP; entityP != NULL; entityP = entityP->next)
  {
    if (entityP->type != KjObject)
    {
      orionldError(OrionldBadRequestData, "EntityInfo array member not a JSON Object", NULL, 400);
      return false;
    }

    KjNode*  itemP;
    char*    idP        = NULL;
    char*    idPatternP = NULL;
    char*    typeP      = NULL;

    for (itemP = entityP->value.firstChildP; itemP != NULL; itemP = itemP->next)
    {
      if (SCOMPARE3(itemP->name, 'i', 'd', 0) || SCOMPARE4(itemP->name, '@', 'i', 'd', 0))
      {
        DUPLICATE_CHECK(idP, "EntityInfo::id", itemP->value.s);
        PCHECK_STRING(itemP, 0, "Not a JSON String", "EntityInfo::id", 400);
        PCHECK_URI(itemP->value.s, true, 0, "Invalid Entity ID", "Not a valid URI", 400);
      }
      else if (SCOMPARE10(itemP->name, 'i', 'd', 'P', 'a', 't', 't', 'e', 'r', 'n', 0))
      {
        DUPLICATE_CHECK(idPatternP, "EntityInfo::idPattern", itemP->value.s);
        STRING_CHECK(itemP, "EntityInfo::idPattern");
      }
      else if (SCOMPARE5(itemP->name, 't', 'y', 'p', 'e', 0) || SCOMPARE6(itemP->name, '@', 't', 'y', 'p', 'e', 0))
      {
        DUPLICATE_CHECK(typeP, "EntityInfo::type", itemP->value.s);
        STRING_CHECK(itemP, "EntityInfo::type");
      }
      else
      {
        orionldError(OrionldBadRequestData, "Unknown EntityInfo field", itemP->name, 400);
        return false;
      }
    }

    if ((idP == NULL) && (idPatternP == NULL) && (typeP == NULL))
    {
      orionldError(OrionldBadRequestData, "Empty EntityInfo object", NULL, 400);
      return false;
    }

    if ((idP != NULL) && (idPatternP != NULL))
    {
      orionldError(OrionldBadRequestData, "Both 'id' and 'idPattern' given in EntityInfo object", NULL, 400);
      return false;
    }

    if (typeP == NULL)
    {
      orionldError(OrionldBadRequestData, "Missing field in EntityInfo object", "type", 400);
      return false;
    }

    ngsiv2::EntID entityInfo;

    if (idP)        entityInfo.id        = idP;
    if (idPatternP) entityInfo.idPattern = idPatternP;

    entityInfo.type = orionldContextItemExpand(orionldState.contextP, typeP, true, NULL);  // Expansion for Regs+Subs
    entitiesP->push_back(entityInfo);
  }

  return true;
}
