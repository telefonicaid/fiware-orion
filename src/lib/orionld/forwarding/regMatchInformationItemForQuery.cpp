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
extern "C"
{
#include "kjson/KjNode.h"                                        // KjNode
#include "kjson/kjLookup.h"                                      // kjLookup
}

#include "orionld/common/orionldState.h"                         // orionldState, kjTreeLog
#include "orionld/regCache/RegCache.h"                           // RegCacheItem
#include "orionld/types/StringArray.h"                           // StringArray
#include "orionld/forwarding/regMatchEntityInfoForQuery.h"       // regMatchEntityInfoForQuery
#include "orionld/forwarding/regMatchAttributesForGet.h"         // regMatchAttributesForGet
#include "orionld/forwarding/regMatchInformationItemForQuery.h"  // Own interface



// -----------------------------------------------------------------------------
//
// regMatchInformationItemForQuery -
//
StringArray* regMatchInformationItemForQuery
(
  RegCacheItem* regP,
  KjNode*       infoP,
  StringArray*  idListP,
  StringArray*  typeListP,
  StringArray*  attrListP,
  KjNode**      entityInfoPP
)
{
  KjNode* entities = kjLookup(infoP, "entities");

  kjTreeLog(infoP, "reg::info array item", LmtRegMatch);

  *entityInfoPP = NULL;  // If no "entities", there can still be a match

  if (entities != NULL)
  {
    bool match = false;
    for (KjNode* entityInfoP = entities->value.firstChildP; entityInfoP != NULL; entityInfoP = entityInfoP->next)
    {
      if (regMatchEntityInfoForQuery(regP, entityInfoP, idListP, typeListP) == true)
      {
        match = true;
        *entityInfoPP = entityInfoP;
        // FIXME: Two different entityInfos might match.
        //        To take care of this case, this function would need to create DistOp's and return a linked list of them
        //        one DistOp per matching item in "entities"
        break;
      }
    }

    if (match == false)
      return NULL;
  }

  KjNode*      propertyNamesP     = kjLookup(infoP, "propertyNames");
  KjNode*      relationshipNamesP = kjLookup(infoP, "relationshipNames");
  StringArray* attrUnionP         = regMatchAttributesForGet(regP, propertyNamesP, relationshipNamesP, attrListP, NULL);

  return attrUnionP;
}
