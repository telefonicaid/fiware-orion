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
#include "kjson/kjLookup.h"                                      // kjLookup
}

#include "orionld/common/orionldError.h"                         // orionldError
#include "orionld/payloadCheck/pCheckAttributeType.h"            // Own interface



// -----------------------------------------------------------------------------
//
// pCheckAttributeType -
//
bool pCheckAttributeType(KjNode* attrP, KjNode** typePP, bool mandatory)
{
  KjNode* typeP   = kjLookup(attrP, "type");
  KjNode* atTypeP = kjLookup(attrP, "@type");

  *typePP = NULL;

  //
  // It's allowed to use either "type" or "@type" but not both
  //
  if ((typeP != NULL) && (atTypeP != NULL))
  {
    orionldError(OrionldBadRequestData, "Duplicate Fields in payload body", "type and @type", 400);
    return false;
  }

  //
  // If "type" is absent, perhaps "@type" is present?
  //
  if (typeP == NULL)
    typeP = atTypeP;

  //
  // If "type" is mandatory but absent - error
  //
  if (typeP == NULL)
  {
    if (mandatory == true)
    {
      orionldError(OrionldBadRequestData, "Missing /type/ field for an attribute", attrP->name, 400);
      return false;
    }

    // If "type" not present, no more checks necessary
    return true;
  }


  //
  // The type field MUST be a string
  //
  if (typeP->type != KjString)
  {
    orionldError(OrionldBadRequestData, "Invalid JSON type for /type/ field of an Attribute (not a JSON String)", attrP->name, 400);
    return false;
  }

  //
  // Save the pointer to the type for later use
  //
  *typePP = typeP;

  return true;
}
