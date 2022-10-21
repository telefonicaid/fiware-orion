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

#include "orionld/common/orionldState.h"                        // orionldState
#include "orionld/common/orionldError.h"                        // orionldError
#include "orionld/common/CHECK.h"                               // STRING_CHECK, ...
#include "orionld/context/orionldAttributeExpand.h"             // orionldAttributeExpand
#include "orionld/payloadCheck/fieldPaths.h"                    // RegistrationInformationEntitiesPath, ...
#include "orionld/payloadCheck/pcheckEntityInfoArray.h"         // pcheckEntityInfoArray
#include "orionld/payloadCheck/pcheckInformationItem.h"         // Own interface



// ----------------------------------------------------------------------------
//
// kjValueInArrayLookup - FIXME: Move to kjTree lib (or kjson library!)
//
// ALSO
//   I think I have a similar function somewhere ...
//
KjNode* kjValueInArrayLookup(KjNode* arrayItemP, const char* value)
{
  while (arrayItemP != NULL)
  {
    if (arrayItemP->type == KjString)
    {
      if (strcmp(arrayItemP->value.s, value) == 0)
        return arrayItemP;
    }

    arrayItemP = arrayItemP->next;
  }

  return NULL;
}



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
  LM(("EXP: In pcheckInformationItem"));
  KjNode* entitiesP      = NULL;
  KjNode* propertiesP    = NULL;
  KjNode* relationshipsP = NULL;

  for (KjNode* infoItemP = informationP->value.firstChildP; infoItemP != NULL; infoItemP = infoItemP->next)
  {
    if (strcmp(infoItemP->name, "entities") == 0)
    {
      DUPLICATE_CHECK(entitiesP, RegistrationInformationEntitiesPath, infoItemP);
      ARRAY_CHECK(entitiesP, RegistrationInformationEntitiesPath);
      EMPTY_ARRAY_CHECK(entitiesP, RegistrationInformationEntitiesPath);
      if (pcheckEntityInfoArray(entitiesP, true, RegistrationInformationEntitiesPathV) == false)
        return false;
    }
    else if ((strcmp(infoItemP->name, "propertyNames") == 0) || (strcmp(infoItemP->name, "properties") == 0))
    {
      DUPLICATE_CHECK(propertiesP, infoItemP->name, infoItemP);
      ARRAY_CHECK(infoItemP, RegistrationInformationPropertyNamesPath);
      EMPTY_ARRAY_CHECK(infoItemP, RegistrationInformationPropertyNamesPath);
      for (KjNode* propP = infoItemP->value.firstChildP; propP != NULL; propP = propP->next)
      {
        STRING_CHECK(propP, RegistrationInformationPropertyNameItemPath);
        EMPTY_STRING_CHECK(propP, RegistrationInformationPropertyNameItemPath);
        LM(("EXP: Expanding property name '%s' (property at %p)", propP->value.s, propP));
        propP->value.s = orionldAttributeExpand(orionldState.contextP, propP->value.s, true, NULL);
      }

      infoItemP->name = (char*) "propertyNames";
    }
    else if ((strcmp(infoItemP->name, "relationshipNames") == 0) || (strcmp(infoItemP->name, "relationships") == 0))
    {
      DUPLICATE_CHECK(relationshipsP, infoItemP->name, infoItemP);
      ARRAY_CHECK(infoItemP, RegistrationInformationRelationshipNamesPath);
      EMPTY_ARRAY_CHECK(infoItemP, RegistrationInformationRelationshipNamesPath);
      for (KjNode* relP = infoItemP->value.firstChildP; relP != NULL; relP = relP->next)
      {
        STRING_CHECK(relP, RegistrationInformationRelationshipNamesItemPath);
        EMPTY_STRING_CHECK(relP, RegistrationInformationRelationshipNamesItemPath);
        LM(("EXP: Expanding relationship name '%s'", relP->value.s));
        relP->value.s = orionldAttributeExpand(orionldState.contextP, relP->value.s, true, NULL);
      }

      infoItemP->name = (char*) "relationshipNames";
    }
    else
    {
      orionldError(OrionldBadRequestData, "Invalid field for Registration::information[X]", infoItemP->name, 400);
      return false;
    }
  }

  if (propertiesP != NULL)
  {
    // Check for duplicates
    for (KjNode* propertyP = propertiesP->value.firstChildP; propertyP != NULL; propertyP = propertyP->next)
    {
      // Start looking for duplicates from the nextcoming name in the array (no need to look back - already done)
      if (kjValueInArrayLookup(propertyP->next, propertyP->value.s) != NULL)
      {
        orionldError(OrionldBadRequestData, "Duplicated Property Name", propertyP->value.s, 400);
        return false;
      }
    }
  }

  if (relationshipsP != NULL)
  {
    // Check for duplicates
    for (KjNode* relationshipP = relationshipsP->value.firstChildP; relationshipP != NULL; relationshipP = relationshipP->next)
    {
      // Start looking for duplicates from the nextcoming name in the array (no need to look back - already done)
      if (kjValueInArrayLookup(relationshipP->next, relationshipP->value.s) != NULL)
      {
        orionldError(OrionldBadRequestData, "Duplicated Relationship Name", relationshipP->value.s, 400);
        return false;
      }
    }
  }

  return true;
}
