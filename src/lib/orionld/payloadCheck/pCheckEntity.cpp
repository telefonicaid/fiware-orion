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
#include "orionld/types/OrionldAttributeType.h"                  // OrionldAttributeType, orionldAttributeType
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
// dbAttributeGet -
//
static KjNode* dbAttributeGet(KjNode* dbEntityP, const char* attributeName, OrionldAttributeType* attributeTypeP)
{
  KjNode* dbAttrsP = kjLookup(dbEntityP, "attrs");   // Really? Look up "attrs" every time?

  if (dbAttrsP == NULL)
    return NULL;

  LM_TMP(("Looking for attribute '%s' in entity", attributeName));
  KjNode* dbAttrP = kjLookup(dbAttrsP, attributeName);

  if (dbAttrP == NULL)
    return NULL;

  KjNode* typeP = kjLookup(dbAttrP, "type");

  if (typeP != NULL)
    *attributeTypeP = orionldAttributeType(typeP->value.s);

  return dbAttrP;
}



// -----------------------------------------------------------------------------
//
// pCheckEntity -
//
// When an entity is created (dbEntityP == NULL), the "type" and "id" are Mandatory:
//   - POST /entities
//   - POST /entityOperations/create
//   - POST /entityOperations/upsert   (possibly)
//
// When an entity is updated:
//   - For the Entity ID:
//     - POST /entities/{entityId}/attrs:    "id" can't be present - it's already in the URL PATH
//     - PATCH /entities/{entityId}/attrs:   "id" can't be present - it's already in the URL PATH
//     - BATCH Upsert/Update:                "id" must be present - can't find the entity otherwise ...
//
//   - For the Entity TYPE:
//     POST /entities/{entityId}/attrs:      if "type" is present, it needs to coincide with what's in the DB
//
bool pCheckEntity
(
  KjNode*  entityP,       // The entity from the incoming payload body
  KjNode*  dbEntityP,     // The entity from the DB, in case the entity already existed
  bool     batch,         // Batch operations have the Entity ID in the payload body - mandatory, Non-batch, the entity-id can't be present
  bool     attrsExpanded  // Attribute names have been expanded already
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

    if (kjLookupByNameExceptOne(entityP, attrP->name, attrP) != NULL)
    {
      orionldError(OrionldBadRequestData, "Duplicated field in an entity", attrP->name, 400);
      return false;
    }

    if (dbEntityP != NULL)  // Get the attribute from the DB
      dbAttributeP = dbAttributeGet(dbEntityP, attrP->name, &attributeType);

    if (pCheckAttribute(attrP, true, dbAttributeP, attributeType, attrsExpanded) == false)
      return false;
  }

  return true;
}
