/*
*
* Copyright 2024 FIWARE Foundation e.V.
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
#include "kjson/kjBuilder.h"                                     // kjChildRemove
}

#include "logMsg/logMsg.h"                                       // LM_*

#include "orionld/apiModel/ntosAttribute.h"                      // Own interface



// -----------------------------------------------------------------------------
//
// ntosAttribute -
//
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
    LM_E(("No attribute value (object/languageMap/json/vocab) found - this should never happen!!!"));

  // Remove sysAttrs of the Attribute
  const char* attrNames[2] = { "createdAt", "modifiedAt" };
  for (int ix = 0; ix < 2; ix++)
  {
    KjNode* nodeP = kjLookup(attrP, attrNames[ix]);
    if (nodeP != NULL)
      kjChildRemove(attrP, nodeP);
  }
}
