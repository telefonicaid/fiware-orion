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
#include <unistd.h>                                            // NULL

extern "C"
{
#include "kjson/KjNode.h"                                      // KjNode
#include "kjson/kjLookup.h"                                    // kjLookup
}

#include "logMsg/logMsg.h"                                     // LM_*
#include "logMsg/traceLevels.h"                                // Lmt*

#include "orionld/troe/PgAppendBuffer.h"                       // PgAppendBuffer
#include "orionld/troe/pgAttributeBuild.h"                     // pgAttributeBuild



// ----------------------------------------------------------------------------
//
// pgAttributesBuild -
//
// The entity id can be passed either as a separate parameter (char* entityId),
// or as part of the attribute list (KjNode* attrList).
// In the latter case, the entityIs nmust be extracted from 'attrList' before the list is treated.
//
void pgAttributesBuild(PgAppendBuffer* attributesBufferP, KjNode* attrList, char* entityId, const char* opMode, PgAppendBuffer* subAttributesBufferP)
{
  if (entityId == NULL)
  {
    KjNode* entityIdNodeP = kjLookup(attrList, "id");

    if (entityIdNodeP == NULL)
      LM_RVE(("Internal Error (entity without id)"));
    else if (entityIdNodeP->type != KjString)
      LM_RVE(("Internal Error (entity id field not a string (%s))", kjValueType(entityIdNodeP->type)));

    entityId = entityIdNodeP->value.s;
  }

  for (KjNode* attrP = attrList->value.firstChildP; attrP != NULL; attrP = attrP->next)
  {
    LM_TMP(("TROE: Treating attribute '%s' (%s)", attrP->name, kjValueType(attrP->type)));

    if (attrP->type == KjArray)
    {
      for (KjNode* aiP = attrP->value.firstChildP; aiP != NULL; aiP = aiP->next)
      {
        aiP->name = attrP->name;
        pgAttributeBuild(attributesBufferP, opMode, entityId, aiP, subAttributesBufferP);
      }
    }
    else if (attrP->type == KjObject)
      pgAttributeBuild(attributesBufferP, opMode, entityId, attrP, subAttributesBufferP);
    else
      LM_TMP(("TROE: Not treating entity-path '%s'", attrP->name));
  }
}
