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
extern "C"
{
#include "kbase/kMacros.h"                                       // K_VEC_SIZE
#include "kalloc/kaAlloc.h"                                      // kaAlloc
#include "kjson/KjNode.h"                                        // KjNode
#include "kjson/kjLookup.h"                                      // kjLookup
#include "kjson/kjBuilder.h"                                     // kjChildRemove, kjString
#include "kjson/kjClone.h"                                       // kjClone
#include "kjson/kjRender.h"                                      // kjFastRender (TEMP)
}

#include "logMsg/logMsg.h"                                       // LM_*

#include "orionld/common/orionldState.h"                         // orionldState
#include "orionld/common/orionldError.h"                         // orionldError
#include "orionld/common/numberToDate.h"                         // numberToDate
#include "orionld/db/dbModelToApiAttribute.h"                    // dbModelToApiAttribute
#include "orionld/db/dbModelToApiEntity.h"                       // Own interface



// -----------------------------------------------------------------------------
//
// dbModelToApiEntity -
//
KjNode* dbModelToApiEntity(KjNode* dbEntityP, bool sysAttrs, const char* entityId)
{
  KjNode* apiEntityP  = kjObject(orionldState.kjsonP, NULL);
  KjNode* dbIdObjectP = kjLookup(dbEntityP, "_id");

  if (dbIdObjectP == NULL)
  {
    orionldError(OrionldInternalError, "Database Error (entity without _id)", NULL, 500);
    return NULL;
  }

  KjNode* dbIdNodeP = kjLookup(dbIdObjectP, "id");
  if (dbIdNodeP != NULL)
  {
    KjNode* idNodeP = kjClone(orionldState.kjsonP, dbIdNodeP);
    kjChildAdd(apiEntityP, idNodeP);
  }
  else
    LM_E(("Database Error (entity '%s' without id inside _id)", entityId));

  KjNode* dbTypeNodeP = kjLookup(dbIdObjectP, "type");
  if (dbTypeNodeP != NULL)
  {
    KjNode* typeNodeP = kjClone(orionldState.kjsonP, dbTypeNodeP);
    kjChildAdd(apiEntityP, typeNodeP);
  }
  else
    LM_E(("Database Error (entity '%s' without type inside _id)", entityId));

  kjChildAdd(apiEntityP, dbTypeNodeP);

  if (sysAttrs)
  {
    const char* fieldName[]  = { "creDate",   "modDate"    };
    const char* ngsildName[] = { "createdAt", "modifiedAt" };

    for (unsigned int ix = 0; ix < K_VEC_SIZE(fieldName); ix++)
    {
      KjNode* nodeP = kjLookup(dbEntityP, fieldName[ix]);
      if (nodeP != NULL)
      {
        char* dateTimeBuf = kaAlloc(&orionldState.kalloc, 32);
        numberToDate(nodeP->value.f, dateTimeBuf, 32);
        nodeP->name       = (char*) ngsildName[ix];
        nodeP->value.s    = dateTimeBuf;
        nodeP->type       = KjString;
      }
    }
  }

  KjNode* dbAttrsP = kjLookup(dbEntityP, "attrs");

  if (dbAttrsP)
  {
    KjNode*  dbAttrP = dbAttrsP->value.firstChildP;
    KjNode*  next;

    while (dbAttrP != NULL)
    {
      next = dbAttrP->next;
      kjChildRemove(dbAttrP, dbAttrsP);
      dbModelToApiAttribute(dbAttrP, sysAttrs);
      kjChildAdd(apiEntityP, dbAttrP);  // No longer a DB attr - has been transformed to API representation
      dbAttrP = next;
    }
  }

  return apiEntityP;
}
