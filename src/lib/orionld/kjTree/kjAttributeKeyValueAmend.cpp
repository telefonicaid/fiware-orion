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
#include "kjson/kjBuilder.h"                                     // kjObject, kjChildAdd, kjChildRemove
#include "kjson/kjLookup.h"                                      // kjLookup
}

#include "logMsg/logMsg.h"                                       // LM_*
#include "logMsg/traceLevels.h"                                  // Lmt*

#include "orionld/types/OrionldAttributeType.h"                  // OrionldAttributeType, orionldAttributeTypeName
#include "orionld/common/orionldState.h"                         // orionldState, coreContextUrl
#include "orionld/common/orionldErrorResponse.h"                 // orionldErrorResponseCreate
#include "orionld/common/attributeTypeFromUriParam.h"            // attributeTypeFromUriParam
#include "orionld/kjTree/kjAttributeKeyValueAmend.h"             // Own interface



// -----------------------------------------------------------------------------
//
// kjAttributeKeyValueAmend -
//
// INPUT:
// {
//   "value": "xxx",  (OR "object" OR "lang" OR ...)
//   "P1": 1,
//   "R1": "urn:..."
// }
//
// OUTPUT:
// {
//   "value/object/...": "xxx",
//   "P1": { "type": "Property", "value": 1 },
//   "R1": { "type": "Relationship", "object": "urn:..." }
// }
//
// If "type" is not given (it is a PATCH operation, so, not necessary), it is looked up in the database.
// We need to know the type, to know whether next we're interested in "value" or "object", or ...
//
// [ "type" CAN ONLY be present in the payload body if it is the same as what is found in the database. ]
//
KjNode* kjAttributeKeyValueAmend(KjNode* attrP, KjNode* dbAttributeP, KjNode** dbAttributeTypePP)
{
  KjNode*  typeP;
  KjNode*  valueP;
  KjNode*  outTreeP = kjObject(orionldState.kjsonP, attrP->name);

  //
  // 1. Look up "type", remove it from the tree and add it to the output tree "outTreeP"
  // 2. Set the valueNodeName according to the value of the type KjNode
  // 3. Look up value/object/..., remove it from the tree and add it to "outTreeP"
  // 4. Go over all sub-attrs, transform them into "non-keyValues" and add to "outTreeP"
  //

  //
  // 1. Look up "type", remove it from the tree and add it to the output tree "outTreeP"
  //
  // If not present, we get it from the DB
  // We need to know the type, to know whether next we're interested in "value" or "object", or ...
  //
  bool typeInPayloadBody = true;
  typeP = kjLookup(attrP, "type");
  if (typeP != NULL)
    kjChildRemove(attrP, typeP);
  else
  {
    typeP = kjLookup(dbAttributeP, "type");
    *dbAttributeTypePP = typeP;
    if (typeP == NULL)
    {
      LM_E(("Database Error (no type field found in the attribute '%s' of an entity)", attrP->name));
      orionldState.httpStatusCode = 500;
      orionldErrorResponseCreate(OrionldInternalError, "no type field found for an attribute", attrP->name);
      return NULL;
    }
    typeInPayloadBody = false;
    kjChildRemove(dbAttributeP, typeP);
  }
  kjChildAdd(outTreeP, typeP);


  //
  // 2. Set the valueNodeName according to the value of the type KjNode
  //
  char* valueNodeName = NULL;
  if      (strcmp(typeP->value.s, "Property")     == 0) valueNodeName = (char*) "value";
  else if (strcmp(typeP->value.s, "GeoProperty")  == 0) valueNodeName = (char*) "value";
  else if (strcmp(typeP->value.s, "Relationship") == 0) valueNodeName = (char*) "object";
  else
  {
    if (typeInPayloadBody == true)
    {
      LM_W(("Bad Input (invalid value of attribute type '%s')", typeP->value.s));
      orionldState.httpStatusCode = 400;
      orionldErrorResponseCreate(OrionldBadRequestData, "Invalid value of attribute type", typeP->value.s);
      return NULL;
    }
    else
    {
      LM_E(("Database Error (invalid value of attribute type '%s')", typeP->value.s));
      orionldState.httpStatusCode = 500;
      orionldErrorResponseCreate(OrionldInternalError, "Invalid value of attribute type", typeP->value.s);
      return NULL;
    }
  }

  //
  // 3. Look up value/object/..., remove it from the tree and add it to "outTreeP"
  //
  // Not Present?
  // That's OK - this is a PATCH operation ... all A-OK
  //
  valueP = kjLookup(attrP, valueNodeName);
  if (valueP != NULL)
  {
    kjChildRemove(attrP, valueP);
    kjChildAdd(outTreeP, valueP);
  }

  //
  // 4. Go over all sub-attrs and transform them into "non-keyValues"
  //    Need a while-loop and a next pointer as the loop moves nodeP around
  //
  KjNode* nodeP = attrP->value.firstChildP;
  KjNode* next;

  while (nodeP != NULL)
  {
    next = nodeP->next;

    kjChildRemove(attrP, nodeP);

    if ((strcmp(nodeP->name, "unitCode") == 0) || (strcmp(nodeP->name, "observedAt") == 0) || (strcmp(nodeP->name, "datasetId") == 0))
    {
      kjChildAdd(outTreeP, nodeP);
      nodeP = next;
      continue;
    }

    KjNode*              objectP       = kjObject(orionldState.kjsonP, nodeP->name);
    OrionldAttributeType attributeType = attributeTypeFromUriParam(nodeP->name);

    //
    // Check the JSON type is OK fore the attribute type.
    // Relationship must be string, LanguageProp must be Object, ...
    //
    if (attributeType == Relationship)
    {
      if (nodeP->type != KjString)
      {
        LM_W(("Bad Input (Relationship attributes must be strings)"));
        orionldState.httpStatusCode = 400;
        orionldErrorResponseCreate(OrionldBadRequestData, "Relationship attributes must be strings", nodeP->name);
        return NULL;
      }
    }
    else if (attributeType == GeoProperty)
    {
      if (nodeP->type != KjObject)
      {
        LM_W(("Bad Input (GeoProperty attributes must be objects)"));
        orionldState.httpStatusCode = 400;
        orionldErrorResponseCreate(OrionldBadRequestData, "GeoProperty attributes must be objects", nodeP->name);
        return NULL;
      }
    }
    else if (attributeType == LanguageProperty)
    {
      if (nodeP->type != KjObject)
      {
        LM_W(("Bad Input (LanguageProperty attributes must be objects)"));
        orionldState.httpStatusCode = 400;
        orionldErrorResponseCreate(OrionldBadRequestData, "LanguageProperty attributes must be objects", nodeP->name);
        return NULL;
      }
    }

    nodeP->name = (attributeType == Relationship)? (char*) "object" : (char*) "value";
    kjChildAdd(objectP, nodeP);

    // Add the type
    KjNode* typeP = kjString(orionldState.kjsonP, "type", orionldAttributeTypeName[attributeType]);
    kjChildAdd(objectP, typeP);

    // Add sub-attr to outgoing tree
    kjChildAdd(outTreeP, objectP);

    // Next!
    nodeP = next;
  }

  return outTreeP;
}
