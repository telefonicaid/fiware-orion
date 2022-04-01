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
}

#include "orionld/common/orionldError.h"                         // orionldError
#include "orionld/payloadCheck/pCheckUri.h"                      // pCheckUri
#include "orionld/payloadCheck/pCheckEntityType.h"               // Own interface



// ----------------------------------------------------------------------------
//
// pCheckEntityType -
//
bool pCheckEntityType(KjNode* typeNodeP, bool mandatory, char** entityTypeP)
{
  if (typeNodeP == NULL)
  {
    if (mandatory == true)
    {
      orionldError(OrionldBadRequestData, "Entity type is missing", "The 'type' field is mandatory", 400);
      return false;
    }

    return true;
  }

  if (typeNodeP->type != KjString)
  {
    orionldError(OrionldBadRequestData, "Entity type must be a String", kjValueType(typeNodeP->type), 400);
    return false;
  }

  if (pCheckUri(typeNodeP->value.s, "Entity Type", false) == false)
    return false;

  if (entityTypeP != NULL)
    *entityTypeP = typeNodeP->value.s;

  return true;
}
