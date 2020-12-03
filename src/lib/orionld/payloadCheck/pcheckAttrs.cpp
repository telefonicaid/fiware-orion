/*
*
* Copyright 2019 FIWARE Foundation e.V.
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
}

#include "logMsg/logMsg.h"                                       // LM_*
#include "logMsg/traceLevels.h"                                  // Lmt*

#include "orionld/common/orionldState.h"                         // orionldState
#include "orionld/common/orionldErrorResponse.h"                 // orionldErrorResponseCreate
#include "orionld/context/orionldContextItemExpand.h"            // orionldContextItemExpand
#include "orionld/payloadCheck/pcheckAttrs.h"                    // Own interface



// -----------------------------------------------------------------------------
//
// pcheckAttrs -
//
bool pcheckAttrs(KjNode* tree)
{
  for (KjNode* attrP = tree->value.firstChildP; attrP != NULL; attrP = attrP->next)
  {
    if (attrP->type != KjString)
    {
      LM_W(("Bad Input (Query 'attrs' array field is not a string: '%s')", kjValueType(attrP->type)));
      orionldErrorResponseCreate(OrionldBadRequestData, "Invalid JSON Content", "Query 'attrs' array field is not a string");
      orionldState.httpStatusCode = 400;
      return false;
    }
    else if (attrP->value.s[0] == 0)
    {
      LM_W(("Bad Input (Query 'attrs' array field is an empty string)"));
      orionldErrorResponseCreate(OrionldBadRequestData, "Invalid JSON Content", "Query 'attrs' array field is an empty string");
      orionldState.httpStatusCode = 400;
      return false;
    }

    attrP->value.s = orionldContextItemExpand(orionldState.contextP, attrP->value.s, true, NULL);
  }

  return true;
}
