/*
*
* Copyright 2019 Telefonica Investigacion y Desarrollo, S.A.U
*
* This file is part of Orion Context Broker.
*
* Orion Context Broker is free software: you can redistribute it and/or
* modify it under the terms of the GNU Affero General Public License as
* published by the Free Software Foundation, either version 3 of the
* License, or (at your option) any later version.
*
* Orion Context Broker is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero
* General Public License for more details.
*
* You should have received a copy of the GNU Affero General Public License
* along with Orion Context Broker. If not, see http://www.gnu.org/licenses/.
*
* For those usages not covered by this license please contact with
* iot_support at tid dot es
*
* Author: Ken Zangelin
*/
extern "C"
{
#include "kjson/KjNode.h"                                        // KjNode
#include "kjson/kjLookup.h"                                      // kjLookup
#include "kalloc/kaStrdup.h"                                     // kaStrdup
}

#include "logMsg/logMsg.h"                                       // LM_*
#include "logMsg/traceLevels.h"                                  // Lmt*

#include "orionld/common/orionldState.h"                         // orionldState
#include "orionld/context/orionldUriExpand.h"                    // orionldUriExpand
#include "orionld/context/orionldValueExpand.h"                  // Own interface



// -----------------------------------------------------------------------------
//
// valueExpand -
//
static void valueExpand(KjNode* nodeP)
{
  char   longName[512];
  char*  detail;

  LM_TMP(("VEX: Expanding '%s' using @context %s", nodeP->value.s, orionldState.contextP->url));
  if (orionldUriExpand(orionldState.contextP, nodeP->value.s, longName, sizeof(longName), NULL, &detail) == false)
  {
    LM_E(("Internal Error (orionldUriExpand failed: %s)", detail));
    return;
  }
  LM_TMP(("VEX: Found it! (%s)", longName));

  LM_TMP(("VEX: Old value: %s", nodeP->value.s));
  nodeP->value.s = kaStrdup(&orionldState.kalloc, longName);
  LM_TMP(("VEX: New value: %s", nodeP->value.s));
}



// -----------------------------------------------------------------------------
//
// orionldValueExpand -
//
// The "value" is only expanded if the type of the value is either KjString or KjArray
//
void orionldValueExpand(KjNode* attrNodeP)
{
  LM_TMP(("VEX: In orionldValueExpand for attribute '%s'", attrNodeP->name));

  KjNode* valueNodeP = kjLookup(attrNodeP, "value");

  LM_TMP(("VEX: Expanding value of attribute '%s'?", attrNodeP->name));

  if (valueNodeP == NULL)
  {
    LM_TMP(("VEX: No value expansion for %s at no @type was found in @context", attrNodeP->name));
    return;
  }
  else if (valueNodeP->type == KjArray)
  {
    LM_TMP(("VEX: Expanding values of array attribute '%s'", attrNodeP->name));
    for (KjNode* nodeP = valueNodeP->value.firstChildP; nodeP != NULL; nodeP = nodeP->next)
    {
      if (nodeP->type == KjString)
      {
        LM_TMP(("VEX: Expanding Array Item String value of attribute '%s'", attrNodeP->name));
        valueExpand(nodeP);
      }
    }
  }
  else if (valueNodeP->type == KjString)
  {
    LM_TMP(("VEX: Expanding String value of attribute '%s'", attrNodeP->name));
    valueExpand(valueNodeP);
  }
  else
    LM_TMP(("VEX: No value expansion for values of type %s", kjValueType(attrNodeP->type)));
}



// -----------------------------------------------------------------------------
//
// orionldDirectValueExpand -
//
char* orionldDirectValueExpand(char* shortName)
{
  char   longName[512];
  char*  detail;

  LM_TMP(("VEX: Expanding '%s' using @context %s", shortName, orionldState.contextP->url));
  if (orionldUriExpand(orionldState.contextP, shortName, longName, sizeof(longName), NULL, &detail) == false)
  {
    LM_E(("Internal Error (orionldUriExpand failed: %s)", detail));
    return NULL;
  }
  LM_TMP(("VEX: Found it! (%s)", longName));

  LM_TMP(("VEX: Old value: %s", shortName));
  return kaStrdup(&orionldState.kalloc, longName);
}
