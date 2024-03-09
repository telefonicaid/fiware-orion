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
#include <string.h>                                              // strcmp

extern "C"
{
#include "kjson/KjNode.h"                                        // KjNode
#include "kjson/kjLookup.h"                                      // kjLookup
#include "kjson/kjBuilder.h"                                     // kjChildRemove
}

#include "logMsg/logMsg.h"                                       // LM_*

#include "orionld/common/orionldState.h"                         // kjTreeLog
#include "orionld/apiModel/ntosEntity.h"                         // Own interface



void ntosAttribute(KjNode* attrP)
{
  bool    asObject  = false;
  KjNode* valueP    = kjLookup(attrP, "value");

  if (valueP == NULL)    { valueP = kjLookup(attrP, "object");                        }
  if (valueP == NULL)    { valueP = kjLookup(attrP, "languageMap"); asObject = true;  }
  if (valueP == NULL)    { valueP = kjLookup(attrP, "json");        asObject = true;  }
  if (valueP == NULL)    { valueP = kjLookup(attrP, "vocab");       asObject = true;  }

  if (valueP != NULL)
  {
    if (asObject == true)
    {
      // Remove everything except valueP (languageMap)
      attrP->value.firstChildP = valueP;
      attrP->lastChild         = valueP;
      valueP->next = NULL;
    }
    else
    {
      attrP->value     = valueP->value;
      attrP->type      = valueP->type;
      attrP->lastChild = valueP->lastChild;
    }
  }
  else
  {
    kjTreeLog(attrP, "attr", LmtBug);
    LM_E(("No attribute value (object/languageMap) found - this should never happen!!!"));
  }

  // Remove sysAttrs of the Attribute
  const char* attrNames[2] = { "createdAt", "modifiedAt" };
  for (int ix = 0; ix < 2; ix++)
  {
    KjNode* nodeP = kjLookup(attrP, attrNames[ix]);
    if (nodeP != NULL)
      kjChildRemove(attrP, nodeP);
  }
}



// -----------------------------------------------------------------------------
//
// ntosEntity - normalized to simplified for an entity
//
// The combination Simplified+KeyValues isn't allowed, we'd never get this far
//
void ntosEntity(KjNode* apiEntityP, const char* lang)
{
  KjNode* attrP = apiEntityP->value.firstChildP;
  KjNode* next;

  // Remove sysAttrs of the Entity
  const char* attrNames[2] = { "createdAt", "modifiedAt" };
  for (int ix = 0; ix < 2; ix++)
  {
    KjNode* nodeP = kjLookup(apiEntityP, attrNames[ix]);
    if (nodeP != NULL)
      kjChildRemove(apiEntityP, nodeP);
  }

  // Loop over the remaining fields
  while (attrP != NULL)
  {
    next = attrP->next;

    if (attrP->type == KjArray)
    {
      for (KjNode* attrInstanceP = attrP->value.firstChildP; attrInstanceP != NULL; attrInstanceP = attrInstanceP->next)
      {
        LM_T(LmtFormat, ("Calling ntosAttribute for '%s', dataset instance X", attrP->name));
        ntosAttribute(attrInstanceP);
      }
    }
    else
    {
      // Skip "id" and "type" ... and "scope" once that gets implemented
      if ((strcmp(attrP->name, "id") != 0) && (strcmp(attrP->name, "type") != 0))
      {
        LM_T(LmtFormat, ("Calling ntosAttribute for '%s'", attrP->name));
        ntosAttribute(attrP);
      }
    }

    attrP = next;
  }
}
