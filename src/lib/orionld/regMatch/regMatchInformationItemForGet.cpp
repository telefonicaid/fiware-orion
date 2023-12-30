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
#include <unistd.h>                                              // NULL

extern "C"
{
#include "kjson/KjNode.h"                                        // KjNode
#include "kjson/kjLookup.h"                                      // kjLookup
}

#include "orionld/types/RegCacheItem.h"                          // RegCacheItem
#include "orionld/types/StringArray.h"                           // StringArray
#include "orionld/regMatch/regMatchEntityInfo.h"                 // regMatchEntityInfo
#include "orionld/regMatch/regMatchAttributesForGet.h"           // regMatchAttributesForGet
#include "orionld/regMatch/regMatchInformationItemForGet.h"      // Own interface



// -----------------------------------------------------------------------------
//
// regMatchInformationItemForGet -
//
StringArray* regMatchInformationItemForGet
(
  RegCacheItem* regP,
  KjNode*       infoP,
  const char*   entityId,
  const char*   entityType,
  StringArray*  attrV,
  const char*   geoProp
)
{
  KjNode* entities = kjLookup(infoP, "entities");

  if (entities != NULL)
  {
    bool match = false;
    for (KjNode* entityInfoP = entities->value.firstChildP; entityInfoP != NULL; entityInfoP = entityInfoP->next)
    {
      if (regMatchEntityInfo(regP, entityInfoP, entityId, entityType) == true)
      {
        match = true;
        break;
      }
    }

    if (match == false)
      return NULL;
  }

  KjNode*      propertyNamesP     = kjLookup(infoP, "propertyNames");
  KjNode*      relationshipNamesP = kjLookup(infoP, "relationshipNames");
  StringArray* attrUnionP         = regMatchAttributesForGet(regP, propertyNamesP, relationshipNamesP, attrV, geoProp);

  return attrUnionP;
}
