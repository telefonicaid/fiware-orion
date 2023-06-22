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
#include "kjson/kjBuilder.h"                                     // kjChildAdd, kjChildRemove
#include "kjson/kjClone.h"                                       // kjClone
}

#include "logMsg/logMsg.h"                                       // LM_*

#include "orionld/common/orionldState.h"                         // orionldState
#include "orionld/notifications/previousValuePopulate.h"         // Own interface



// -----------------------------------------------------------------------------
//
// previousValuePopulate -
//
void previousValuePopulate(KjNode* dbAttrsP, KjNode* dbAttrP, const char* attrName)
{
  if (orionldState.previousValues == NULL)
    orionldState.previousValues = kjObject(orionldState.kjsonP, NULL);

  if ((dbAttrP == NULL) && (dbAttrsP != NULL))
    dbAttrP = kjLookup(dbAttrsP, attrName);

  if (dbAttrP != NULL)
  {
    KjNode* valueP = kjLookup(dbAttrP, "value");
    if (valueP != NULL)
    {
      KjNode* prevValueP = kjClone(orionldState.kjsonP, valueP);
      kjChildAdd(orionldState.previousValues, prevValueP);
      prevValueP->name = (char*) attrName;
      LM_T(LmtShowChanges, ("Added previousValue for '%s' in orionldState.previousValues", attrName));
    }
  }
}
