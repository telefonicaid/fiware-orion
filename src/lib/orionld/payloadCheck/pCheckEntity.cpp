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
#include "orionld/common/dotForEq.h"                             // dorForEq
#include "orionld/types/OrionldAttributeType.h"                  // OrionldAttributeType, orionldAttributeType
#include "orionld/context/orionldContextItemExpand.h"            // orionldContextItemExpand
#include "orionld/context/orionldAttributeExpand.h"              // orionldAttributeExpand
#include "orionld/kjTree/kjTreeLog.h"                            // kjTreeLog
#include "orionld/payloadCheck/PCHECK.h"                         // PCHECK_*
#include "orionld/payloadCheck/pcheckName.h"                     // pCheckName
#include "orionld/payloadCheck/pCheckUri.h"                      // pCheckUri
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
// idCheck -
//
static bool idCheck(KjNode* attrP, KjNode* idP)
{
  if (idP != NULL)
  {
    // We know that there must have been an "id" and an "@id", as duplicated fields are detected already by kjLookupByNameExceptOne
    orionldError(OrionldBadRequestData, "Duplicated field in an entity (id+@id)", idP->value.s, 400);
    return false;
  }

  PCHECK_STRING(attrP,             0, NULL, "The Entity ID must be a string that is a valid URI", 400);
  PCHECK_URI(attrP->value.s, true, 0, NULL, "The Entity ID must be a valid URI",                  400);

  return true;
}



// -----------------------------------------------------------------------------
//
// typeCheck -
//
static bool typeCheck(KjNode* attrP, KjNode* typeP, KjNode* idP)
{
  if (typeP != NULL)
  {
    // We know that there must have been a "type" and an "@type", as duplicated fields are detected already by kjLookupByNameExceptOne
    if (idP != NULL)
      orionldError(OrionldBadRequestData, "Duplicated field in an entity (type+@type)", idP->value.s, 400);
    else
      orionldError(OrionldBadRequestData, "Duplicated field in an entity", "type+@type", 400);
    return false;
  }

  PCHECK_STRING(attrP,              0, "The Entity Type must be a JSON String", kjValueType(attrP->type), 400);
  PCHECK_URI(attrP->value.s, false, 0, "Invalid URI",                           "Invalid Entity Type",    400);

  return true;
}



// -----------------------------------------------------------------------------
//
// attrTypeFromDb -
//
// Look up the attribute in dbAttrsP, extract the attribute type and pass it to pCheckAttribute
//
static OrionldAttributeType attrTypeFromDb(KjNode* dbAttrsP, char* attrName)
{
  KjNode* attrP = kjLookup(dbAttrsP, attrName);

  if (attrP == NULL)
    return NoAttributeType;

  KjNode* typeP = kjLookup(attrP, "type");
  if (typeP == NULL)
    return NoAttributeType;  // Really a DB Error but ... best effort?

  return orionldAttributeType(typeP->value.s);
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
  bool     batch,         // Batch operations have the Entity ID in the payload body - mandatory, Non-batch, the entity-id can't be present
  KjNode*  dbAttrsP       // "attrs" member - all attributes - from database
)
{
  // Remove builtin timestamps, if present
  KjNode* nodeP;

  if ((nodeP = kjLookup(entityP, "createdAt"))  != NULL)  kjChildRemove(entityP, nodeP);
  if ((nodeP = kjLookup(entityP, "modifiedAt")) != NULL)  kjChildRemove(entityP, nodeP);

  // Loop over attributes
  KjNode* idP   = NULL;
  KjNode* typeP = NULL;

  for (KjNode* attrP = entityP->value.firstChildP; attrP != NULL; attrP = attrP->next)
  {
    if (kjLookupByNameExceptOne(entityP, attrP->name, attrP) != NULL)
    {
      orionldError(OrionldBadRequestData, "Duplicated field in an entity", attrP->name, 400);
      return false;
    }

    //
    // id
    //
    if ((strcmp(attrP->name, "id") == 0) || (strcmp(attrP->name, "@id") == 0))
    {
      if (idCheck(attrP, idP) == false)  // POST /entities/*/attrs   CANNOT add/modify "id"
        return false;

      idP = attrP;
      continue;
    }

    //
    // type
    //
    if ((strcmp(attrP->name, "type")  == 0) || (strcmp(attrP->name, "@type") == 0))
    {
      if (typeCheck(attrP, typeP, idP) == false)
        return false;

      typeP          = attrP;
      typeP->value.s = orionldContextItemExpand(orionldState.contextP, typeP->value.s, true, NULL);
      continue;
    }

    //
    // Special attributes
    //
    if (strcmp(attrP->name, "@context") == 0)      continue;
    if (strcmp(attrP->name, "scope")    == 0)      continue;


    OrionldAttributeType  aTypeFromDb  = NoAttributeType;
    OrionldContextItem*   contextItemP = NULL;

    //
    // Before expanding we must check the validity of the attribute name
    //
    if (pCheckName(attrP->name) == false)
      return false;
    if (pCheckUri(attrP->name, attrP->name, false) == false)  // FIXME: Both pCheckName and pCheckUri check for forbidden chars ...
      return false;
    attrP->name = orionldAttributeExpand(orionldState.contextP, attrP->name, true, &contextItemP);

    if (dbAttrsP != NULL)
    {
      char* attrName = kaStrdup(&orionldState.kalloc, attrP->name);
      dotForEq(attrName);
      aTypeFromDb = attrTypeFromDb(dbAttrsP, attrName);
    }

    if (pCheckAttribute(attrP, true, aTypeFromDb, true, contextItemP) == false)
      return false;
  }

  //
  // Remove the possible '@' for Entity "id" and "type"
  //
  if (idP   != NULL) idP->name   = (char*) "id";
  if (typeP != NULL) typeP->name = (char*) "type";

  // If batch or POST /entities - idP cannot be NULL
  // - All other operations, it must be NULL (can't be present)

  // If batch create or POST /entities - typeP cannot be NULL  (also true if batch upert that is a create)
  // If not creation, type cannot be present (until multi-type is implemented)

  return true;
}
