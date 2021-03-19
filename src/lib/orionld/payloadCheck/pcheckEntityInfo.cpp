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
#include "kjson/KjNode.h"                                       // KjNode
}

#include "logMsg/logMsg.h"                                      // LM_*
#include "logMsg/traceLevels.h"                                 // Lmt*

#include "orionld/common/CHECK.h"                               // STRING_CHECK, ...
#include "orionld/common/orionldState.h"                        // orionldState
#include "orionld/common/orionldErrorResponse.h"                // orionldErrorResponseCreate
#include "orionld/context/orionldContextItemExpand.h"           // orionldContextItemExpand
#include "orionld/context/orionldContextItemAlreadyExpanded.h"  // orionldContextItemAlreadyExpanded
#include "orionld/payloadCheck/pcheckEntityInfo.h"              // Own interface



// ----------------------------------------------------------------------------
//
// pcheckEntityInfo -
//
bool pcheckEntityInfo(KjNode* entityInfoP, bool typeMandatory)
{
  KjNode* idP         = NULL;
  KjNode* idPatternP  = NULL;
  KjNode* typeP       = NULL;

  OBJECT_CHECK(entityInfoP, "entities[X]");
  EMPTY_OBJECT_CHECK(entityInfoP, "entities[X]");

  for (KjNode* entityItemP = entityInfoP->value.firstChildP; entityItemP != NULL; entityItemP = entityItemP->next)
  {
    if ((strcmp(entityItemP->name, "id") == 0) || (strcmp(entityItemP->name, "@id") == 0))
    {
      DUPLICATE_CHECK(idP, "entities[X]::id", entityItemP);
      STRING_CHECK(entityItemP, "entities[X]::id");
      EMPTY_STRING_CHECK(entityItemP, "entities[X]::id");
      URI_CHECK(entityItemP, "entities[X]::id");
    }
    else if (strcmp(entityItemP->name, "idPattern") == 0)
    {
      DUPLICATE_CHECK(idPatternP, "entities[X]::idPattern", entityItemP);
      STRING_CHECK(entityItemP, "entities[X]::idPattern");
      EMPTY_STRING_CHECK(entityItemP, "entities[X]::idPattern");
      // FIXME: How check for valid REGEX???
    }
    else if ((strcmp(entityItemP->name, "type") == 0) || (strcmp(entityItemP->name, "@type") == 0))
    {
      DUPLICATE_CHECK(typeP, "entities[X]::type", entityItemP);
      STRING_CHECK(entityItemP, "entities[X]::type");
      EMPTY_STRING_CHECK(entityItemP, "entities[X]::type");

      //
      // Expand, unless already expanded
      // If a ':' is found inside the first 10 chars, the value is assumed to be expanded ...
      //
      if (orionldContextItemAlreadyExpanded(entityItemP->value.s) == false)
        entityItemP->value.s = orionldContextItemExpand(orionldState.contextP, entityItemP->value.s, true, NULL);  // entity type
    }
    else
    {
      orionldErrorResponseCreate(OrionldBadRequestData, "Invalid field for entities[X]", entityItemP->name);
      orionldState.httpStatusCode = SccBadRequest;
      return false;
    }
  }

  // Only if Fully NGSI-LD compliant
  if ((typeMandatory == true) && (typeP == NULL))
  {
    orionldErrorResponseCreate(OrionldBadRequestData, "Missing mandatory field", "entities[X]::type");
    orionldState.httpStatusCode = SccBadRequest;
    return false;
  }

  return true;
}
