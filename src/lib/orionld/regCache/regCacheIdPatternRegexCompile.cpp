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
#include <regex.h>                                               // regcomp

extern "C"
{
#include "kjson/KjNode.h"                                        // KjNode
#include "kjson/kjLookup.h"                                      // kjLookup
}

#include "logMsg/logMsg.h"                                       // LM_*

#include "orionld/types/OrionldTenant.h"                         // OrionldTenant
#include "orionld/types/RegCacheItem.h"                          // RegCacheItem
#include "orionld/regCache/regCacheIdPatternRegexCompile.h"      // Own interface



// -----------------------------------------------------------------------------
//
// regCacheIdPatternRegexCompile -
//
bool regCacheIdPatternRegexCompile(RegCacheItem* rciP, KjNode* informationArrayP)
{
  for (KjNode* informationP = informationArrayP->value.firstChildP; informationP != NULL; informationP = informationP->next)
  {
    KjNode* entitiesP = kjLookup(informationP, "entities");

    if (entitiesP == NULL)
      continue;

    for (KjNode* entityInfoP = entitiesP->value.firstChildP; entityInfoP != NULL; entityInfoP = entityInfoP->next)
    {
      KjNode* idPatternP = kjLookup(entityInfoP, "idPattern");

      if (idPatternP == NULL)
        continue;

      //
      // Got an idPattern - need to create the regex and then store it and a reference to this specific idPattern
      // in the registration cache.
      //
      RegIdPattern* ripP = (RegIdPattern*) malloc(sizeof(RegIdPattern));

      if (ripP == NULL)
        LM_X(1, ("Out of memory"));

      ripP->owner = idPatternP;
      if (regcomp(&ripP->regex, idPatternP->value.s, REG_EXTENDED) != 0)
      {
        LM_E(("Error compiling a regular expression for '%s'", idPatternP->value.s));
        return false;
      }

      // Enqueue the RegIdPattern
      ripP->next = rciP->idPatternRegexList;
      rciP->idPatternRegexList = ripP;
    }
  }

  return true;
}
