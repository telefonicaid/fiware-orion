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
#include "kjson/KjNode.h"                                        // KjNode
#include "kjson/kjLookup.h"                                      // kjLookup
#include "kjson/kjBuilder.h"                                     // kjChildRemove
}

#include "logMsg/logMsg.h"                                       // LM_*

#include "orionld/types/OrionldAttributeType.h"                  // OrionldAttributeType
#include "orionld/kjTree/kjChildCount.h"                         // kjChildCount
#include "orionld/kjTree/kjAttributeNormalizedToConcise.h"       // Own interface



// ----------------------------------------------------------------------------
//
// kjAttributeNormalizedToConcise -
//
// FIXME - steal stuff from kjEntityNormalizedToConcise
//
void kjAttributeNormalizedToConcise(KjNode* attrP, const char* lang)
{
  OrionldAttributeType attrType = NoAttributeType;
  KjNode*              typeP    = kjLookup(attrP, "type");

  LM_TMP(("In kjAttributeNormalizedToConcise"));

  if (typeP == NULL)
    LM_RVE(("The attribute '%s' doesn't have an attribute type", attrP->name));
  if (typeP->type != KjString)
    LM_RVE(("The attribute '%s' has an attribute type that is not a String", attrP->name));

  kjChildRemove(attrP, typeP);
  attrType = orionldAttributeType(typeP->value.s);

  if ((attrType == Property) || (attrType == GeoProperty))
  {
    if (kjChildCount(attrP) == 1)
    {
      attrP->type  = attrP->value.firstChildP->type;
      attrP->value = attrP->value.firstChildP->value;
      return;
    }
  }

  // Loop over sub-attributes
  for (KjNode* subAttrP = attrP->value.firstChildP; subAttrP != NULL; subAttrP = subAttrP->next)
  {
    if (strcmp(subAttrP->name, "value")       == 0)     continue;
    if (strcmp(subAttrP->name, "object")      == 0)     continue;  // FIXME: not sure ...
    if (strcmp(subAttrP->name, "languageMap") == 0)     continue;  // FIXME: not sure ...

    kjAttributeNormalizedToConcise(subAttrP, lang);
  }
}
