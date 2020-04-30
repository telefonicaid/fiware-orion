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

#include "logMsg/logMsg.h"                                      // LM_*
#include "logMsg/traceLevels.h"                                 // Lmt*

#include "orionld/common/orionldState.h"                        // orionldState
#include "orionld/common/orionldErrorResponse.h"                // orionldErrorResponseCreate
#include "orionld/common/CHECK.h"                               // STRING_CHECK, ...
#include "orionld/payloadCheck/pcheckInformationItem.h"         // pcheckInformationItem
#include "orionld/payloadCheck/pcheckInformation.h"             // Own interface



// -----------------------------------------------------------------------------
//
// pcheckInformation -
//
bool pcheckInformation(KjNode* informationArrayP)
{
  if (informationArrayP->value.firstChildP == NULL)  // Empty Array
  {
    orionldErrorResponseCreate(OrionldBadRequestData, "Empty Array", "information");
    orionldState.httpStatusCode = SccBadRequest;
    return false;
  }

  for (KjNode* informationP = informationArrayP->value.firstChildP; informationArrayP != NULL; informationArrayP = informationArrayP->next)
  {
    OBJECT_CHECK(informationP, "information[X]");

    if (pcheckInformationItem(informationP) == false)
      return false;
  }

  return true;
}
