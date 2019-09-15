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
#include "kjson/KjNode.h"                                      // KjNode
}

#include "logMsg/logMsg.h"                                     // LM_*
#include "logMsg/traceLevels.h"                                // Lmt*

#include "rest/ConnectionInfo.h"                               // ConnectionInfo
#include "orionld/common/orionldErrorResponse.h"               // orionldErrorResponseCreate
#include "orionld/common/OrionldConnection.h"                  // orionldState



// ----------------------------------------------------------------------------
//
// orionldContextInlineCheck -
//
bool orionldContextInlineCheck(ConnectionInfo* ciP, KjNode* contextObjectP)
{
  KjNode* nodeP;

  for (nodeP = contextObjectP->value.firstChildP; nodeP != NULL; nodeP = nodeP->next)
  {
    if ((nodeP->type != KjString) && (nodeP->type != KjObject))
    {
      LM_E(("The context is invalid - value of '%s' is not a String nor an Object", nodeP->name));
      orionldState.contextP = NULL;  // Leak?
      orionldErrorResponseCreate(OrionldBadRequestData, "Invalid key-value in @context", nodeP->name, OrionldDetailString);
      ciP->httpStatusCode = SccBadRequest;
      return false;
    }
  }

  return true;
}
