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

#include "orionld/common/CHECK.h"                               // STRING_CHECK, ...
#include "orionld/common/orionldState.h"                        // orionldState
#include "orionld/common/orionldErrorResponse.h"                // orionldErrorResponseCreate
#include "orionld/context/orionldContextItemAlreadyExpanded.h"  // orionldContextItemAlreadyExpanded
#include "orionld/context/orionldContextItemExpand.h"           // orionldContextItemExpand
#include "orionld/payloadCheck/pcheckEntityInfoArray.h"         // pcheckEntityInfoArray
#include "orionld/payloadCheck/pcheckInformationItem.h"         // Own interface



// ----------------------------------------------------------------------------
//
// pcheckInformationItem -
//
// The "information" field can have only three members:
//   * entities       (Mandatory JSON Array)
//   * properties     (Optional JSON Array with strings)
//   * relationships  (Optional JSON Array with strings)
//
// NOTE
//   This function also expands ATTRIBUTE NAMES and ENTITY TYPES
//
bool pcheckInformationItem(KjNode* informationP)
{
  KjNode* entitiesP      = NULL;
  KjNode* propertiesP    = NULL;
  KjNode* relationshipsP = NULL;

  for (KjNode* infoItemP = informationP->value.firstChildP; infoItemP != NULL; infoItemP = infoItemP->next)
  {
    if (strcmp(infoItemP->name, "entities") == 0)
    {
      DUPLICATE_CHECK(entitiesP, "entities", infoItemP);
      ARRAY_CHECK(entitiesP, "entities");
      EMPTY_ARRAY_CHECK(entitiesP, "entities");
      if (pcheckEntityInfoArray(entitiesP, true) == false)  // FIXME: Why is the entity type mandatory?
        return false;
    }
    else if (strcmp(infoItemP->name, "properties") == 0)
    {
      DUPLICATE_CHECK(propertiesP, "properties", infoItemP);
      ARRAY_CHECK(infoItemP, "information[X]::properties");
      EMPTY_ARRAY_CHECK(infoItemP, "information[X]::properties");
      for (KjNode* propP = infoItemP->value.firstChildP; propP != NULL; propP = propP->next)
      {
        STRING_CHECK(propP, "information[X]::properties[X]");
        EMPTY_STRING_CHECK(propP, "information[X]::properties[X]");
        if (orionldContextItemAlreadyExpanded(propP->value.s) == false)
          propP->value.s = orionldContextItemExpand(orionldState.contextP, propP->value.s, true, NULL);
      }
    }
    else if (strcmp(infoItemP->name, "relationships") == 0)
    {
      DUPLICATE_CHECK(relationshipsP, "relationships", infoItemP);
      ARRAY_CHECK(infoItemP, "information[X]::relationships");
      EMPTY_ARRAY_CHECK(infoItemP, "information[X]::relationships");
      for (KjNode* relP = infoItemP->value.firstChildP; relP != NULL; relP = relP->next)
      {
        STRING_CHECK(relP, "information[X]::relationships[X]");
        EMPTY_STRING_CHECK(relP, "information[X]::relationships[X]");
        if (orionldContextItemAlreadyExpanded(relP->value.s) == false)
          relP->value.s = orionldContextItemExpand(orionldState.contextP, relP->value.s, true, NULL);
      }
    }
    else
    {
      orionldErrorResponseCreate(OrionldBadRequestData, "Invalid field for information[X]", infoItemP->name);
      orionldState.httpStatusCode = SccBadRequest;
      return false;
    }
  }

  return true;
}
