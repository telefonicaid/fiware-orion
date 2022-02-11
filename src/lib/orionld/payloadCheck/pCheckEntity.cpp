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
#include <string.h>                                              // strcmp

extern "C"
{
#include "kalloc/kaStrdup.h"                                     // kaStrdup
#include "kjson/KjNode.h"                                        // KjNode
#include "kjson/kjLookup.h"                                      // kjLookup
#include "kjson/kjBuilder.h"                                     // kjString, kjObject, ...
}

#include "logMsg/logMsg.h"                                       // LM_*

#include "orionld/common/orionldState.h"                         // orionldState
#include "orionld/common/orionldError.h"                         // orionldError
#include "orionld/payloadCheck/pCheckAttribute.h"                // pCheckAttribute
#include "orionld/payloadCheck/pCheckEntity.h"                   // Own interface


// -----------------------------------------------------------------------------
//
// kjLookupByNameExceptOne -
//
KjNode* kjLookupByNameExceptOne(KjNode* containerP, const char* fieldName, KjNode* exceptP)
{
  if (containerP->type != KjObject)
    return NULL;  // BUG ?

  for (KjNode* fieldP = containerP->value.firstChildP; fieldP != NULL; fieldP = fieldP->next)
  {
    if (fieldP == exceptP)
      continue;

    if (strcmp(fieldP->name, fieldName) == 0)
      return fieldP;
  }

  return NULL;
}



// -----------------------------------------------------------------------------
//
// pCheckEntity -
//
bool pCheckEntity
(
  KjNode*  entityP,    // The entity from the incoming payload body
  KjNode*  dbEntityP,  // The entity from the DB, in case the entity already existed
  KjNode*  idNodeP,    // Entity ID
  KjNode*  typeNodeP   // Entity Type
)
{
  KjNode* nodeP;

  // Remove builtin timestamps, if present
  if ((nodeP = kjLookup(entityP, "createdAt"))  != NULL)  kjChildRemove(entityP, nodeP);
  if ((nodeP = kjLookup(entityP, "modifiedAt")) != NULL)  kjChildRemove(entityP, nodeP);

  // Loop over attributes
  for (KjNode* attrP = entityP->value.firstChildP; attrP != NULL; attrP = attrP->next)
  {
    OrionldAttributeType  attributeType = NoAttributeType;
    KjNode*               dbAttributeP  = NULL;

    if (dbEntityP != NULL)
    {
      // Get the type for the attribute, if it exists already (in the DB)
    }

    if (kjLookupByNameExceptOne(entityP, attrP->name, attrP) != NULL)
    {
      orionldError(OrionldBadRequestData, "Duplicated field in an entity", attrP->name, 400);
      return false;
    }

    if (pCheckAttribute(attrP, true, dbAttributeP, attributeType) == false)
      return false;
  }

  return true;
}
