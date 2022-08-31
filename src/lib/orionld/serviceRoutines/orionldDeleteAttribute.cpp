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
extern "C"
{
#include "kjson/KjNode.h"                                        // KjNode
#include "kjson/kjLookup.h"                                      // kjLookup
}

#include "logMsg/logMsg.h"                                       // LM_*

#include "orionld/common/orionldState.h"                         // orionldState
#include "orionld/common/orionldError.h"                         // orionldError
#include "orionld/kjTree/kjTreeLog.h"                            // kjTreeLog
#include "orionld/kjTree/kjStringValueLookupInArray.h"           // kjStringValueLookupInArray
#include "orionld/legacyDriver/legacyDeleteAttribute.h"          // legacyDeleteAttribute
#include "orionld/payloadCheck/pCheckUri.h"                      // pCheckUri
#include "orionld/mongoc/mongocEntityGet.h"                      // mongocEntityGet
#include "orionld/mongoc/mongocAttributeDelete.h"                // mongocAttributeDelete
#include "orionld/serviceRoutines/orionldDeleteAttribute.h"      // Own interface



// ----------------------------------------------------------------------------
//
// orionldDeleteAttribute -
//
bool orionldDeleteAttribute(void)
{
  if ((experimental == false) || (orionldState.in.legacy != NULL))                      // If Legacy header - use old implementation
    return legacyDeleteAttribute();

  char* entityId = orionldState.wildcard[0];
  char* attrName = orionldState.wildcard[1];

  //
  // Make sure the Entity ID is a valid URI
  //
  if (pCheckUri(entityId, "Entity ID from URL PATH", true) == false)
    return false;

  //
  // Make sure the Attribute Name is valid
  //
  if (pCheckUri(attrName, "Attribute Name from URL PATH", false) == false)
    return false;


  //
  // orionldMhdConnectionTreat() expands the attribute name for us.
  // Here we save it in the orionldState.wildcard array, so that TRoE won't have to expand it
  //
  orionldState.wildcard[1] = orionldState.in.pathAttrExpanded;

  //
  // Retrieve part of the entity from the database (only attrNames)
  //
  const char* projection[] = { "attrNames", NULL };
  KjNode*     dbEntityP    = mongocEntityGet(entityId, projection, false);

  if (dbEntityP == NULL)
  {
    orionldError(OrionldResourceNotFound, "Entity Not Found", entityId, 404);
    return false;
  }

  kjTreeLog(dbEntityP, "dbEntityP");
  KjNode* attrNamesP = kjLookup(dbEntityP, "attrNames");
  if (attrNamesP == NULL)
  {
    orionldError(OrionldInternalError, "Database Error (attrNames field not present in database)", entityId, 500);
    return false;
  }

  KjNode* attrNameP = kjStringValueLookupInArray(attrNamesP, orionldState.in.pathAttrExpanded);
  if (attrNameP == NULL)
  {
    orionldError(OrionldResourceNotFound, "Entity/Attribute not found", orionldState.in.pathAttrExpanded, 404);
    return false;
  }

  int r = mongocAttributeDelete(entityId, orionldState.in.pathAttrExpanded);
  if (r == false)
  {
    orionldError(OrionldInternalError, "Database Error (deleting attribute from entity)", entityId, 500);
    return false;
  }

  orionldState.httpStatusCode = 204;
  orionldState.requestTree    = NULL;

  return true;
}
