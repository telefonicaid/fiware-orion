/*
*
* Copyright 2021 FIWARE Foundation e.V.
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
#include <unistd.h>                                               // NULL

extern "C"
{
#include "kalloc/kaStrdup.h"                                      // kaStrdup
#include "kjson/KjNode.h"                                         // KjNode
#include "kjson/kjBuilder.h"                                      // kjArray, kjObject
#include "kjson/kjLookup.h"                                       // kjLookup
#include "kjson/kjClone.h"                                        // kjClone
#include "kjson/kjRender.h"                                       // kjFastRender
}

#include "logMsg/logMsg.h"                                        // LM_*
#include "logMsg/traceLevels.h"                                   // Lmt*

#include "orionld/types/OrionldProblemDetails.h"                  // OrionldProblemDetails, orionldProblemDetailsFill
#include "orionld/common/orionldState.h"                          // orionldState
#include "orionld/common/uuidGenerate.h"                          // uuidGenerate
#include "orionld/common/orionldErrorResponse.h"                  // orionldErrorResponseCreate
#include "orionld/common/eqForDot.h"                              // eqForDot
#include "orionld/common/dotForEq.h"                              // dotForEq
#include "orionld/context/orionldContextItemAliasLookup.h"        // orionldContextItemAliasLookup
#include "orionld/kjTree/kjStringValueLookupInArray.h"            // kjStringValueLookupInArray
#include "orionld/kjTree/kjStringArraySortedInsert.h"             // kjStringArraySortedInsert
#include "orionld/db/dbConfiguration.h"                           // dbEntityAttributesFromRegistrationsGet, dbEntitiesGet
#include "orionld/db/dbEntityAttributesGet.h"                     // Own interface



// -----------------------------------------------------------------------------
//
// getEntityAttributesResponse - All entity attributes for which entity instances are currently available in the NGSI-LD system.
//
static KjNode* getEntityAttributesResponse(KjNode* sortedArrayP)
{
  char entityAttributesId[128];

  strncpy(entityAttributesId, "urn:ngsi-ld:EntityAttributeList:", sizeof(entityAttributesId) - 1);
  uuidGenerate(&entityAttributesId[32], sizeof(entityAttributesId) - 32, false);

  KjNode* attributeNodeResponseP = kjObject(orionldState.kjsonP, NULL);
  KjNode* idNodeP                = kjString(orionldState.kjsonP, "id", entityAttributesId);
  KjNode* typeNodeP              = kjString(orionldState.kjsonP, "type", "EntityAttributeList");

  kjChildAdd(attributeNodeResponseP, idNodeP);
  kjChildAdd(attributeNodeResponseP, typeNodeP);
  kjChildAdd(attributeNodeResponseP, sortedArrayP);

  return attributeNodeResponseP;
}



// ----------------------------------------------------------------------------
//
// localAttrNamesExtract -
//
// PARAMETERS
// - outArray: The result of the operation
// - local:    The output from dbEntitiesGet(attrNames), which is an array of the attrNames field of matching entities, e.g.:
//               [
//                 { "attrNames": ["https://uri.etsi.org/ngsi-ld/default-context/P1","https://uri.etsi.org/ngsi-ld/default-context/P2"] },
//                 { "attrNames": ["https://uri.etsi.org/ngsi-ld/default-context/P2","https://uri.etsi.org/ngsi-ld/default-context/R1"] },
//               ]
//
// What we need to do now is to extract all "attrNames" from the objects in the array and
// - loop over the attribute names
// - lookup the alias
// - sorted insert into a new array - the output array (first lookup to we have no duplicates)
//
static void localAttrNamesExtract(KjNode* outArray, KjNode* local)
{
  for (KjNode* objP = local->value.firstChildP; objP != NULL; objP = objP->next)
  {
    KjNode* attrNamesP = kjLookup(objP, "attrNames");

    if (attrNamesP == NULL)
    {
      LM_E(("Database Error (entity:attrNames not present in DB)"));
      continue;
    }

    if (attrNamesP->type != KjArray)
    {
      LM_E(("Database Error (entity:attrNames present but not an Array: '%s')", kjValueType(attrNamesP->type)));
      continue;
    }

    KjNode* aP = attrNamesP->value.firstChildP;
    KjNode* next;

    while (aP != NULL)
    {
      next = aP->next;

      if (aP->type != KjString)
      {
        LM_E(("Database Error (entity:attrNames item is not a String (it is of type '%s')", kjValueType(aP->type)));
        aP = next;
        continue;
      }

      aP->value.s = orionldContextItemAliasLookup(orionldState.contextP, aP->value.s, NULL, NULL);

      if (kjStringValueLookupInArray(outArray, aP->value.s) == NULL)
      {
        kjChildRemove(objP, aP);
        kjStringArraySortedInsert(outArray, aP);
      }

      aP = next;
    }
  }
}



// ----------------------------------------------------------------------------
//
// remoteAttrNamesExtract -
//
// PARAMETERS
// - outArray: The result of the operation
// - remote:   The output from dbEntityTypesFromRegistrationsGet(details=true), which is an array of
//             [
//               {
//                "id": "urn:ngsi-ld:entity:E1",
//                "type": "https://uri.etsi.org/ngsi-ld/default-context/T1",
//                "attrs": ["https://uri.etsi.org/ngsi-ld/default-context/brandName", "https://uri.etsi.org/ngsi-ld/default-context/speed"]
//               },
//               {
//                 "id": "urn:ngsi-ld:entity:E2",
//                 "type": "https://uri.etsi.org/ngsi-ld/default-context/T2",
//                 "attrs": ["https://uri.etsi.org/ngsi-ld/default-context/isParked"]
//               }
//             ]
//
// What we need to do now is to extract all strings in the "attrs" arrays of all the objects in the toplevel array
// - loop over the attribute names
// - lookup the alias
// - sorted insert into a new array - the output array (first lookup to we have no duplicates)
//
static void remoteAttrNamesExtract(KjNode* outArray, KjNode* remote)
{
  for (KjNode* objP = remote->value.firstChildP; objP != NULL; objP = objP->next)
  {
    KjNode* attrsArray = kjLookup(objP, "attrs");

    if (attrsArray != NULL)
    {
      KjNode* attrNameNode = attrsArray->value.firstChildP;
      KjNode* next;

      while (attrNameNode != NULL)
      {
        next = attrNameNode->next;

        attrNameNode->value.s = orionldContextItemAliasLookup(orionldState.contextP, attrNameNode->value.s, NULL, NULL);

        if (kjStringValueLookupInArray(outArray, attrNameNode->value.s) == NULL)
        {
          kjChildRemove(objP, attrNameNode);
          kjStringArraySortedInsert(outArray, attrNameNode);
        }

        attrNameNode = next;
      }
    }
  }
}



// -----------------------------------------------------------------------------
//
// dbEntityAttributesGetWithoutDetails -
//
static KjNode* dbEntityAttributesGetWithoutDetails(OrionldProblemDetails* pdP)
{
  //
  // GET local attributes - i.e. from the "entities" collection
  //
  KjNode* localEntityArray;
  char*   fields[1] = { (char*) "attrNames" };

  localEntityArray = dbEntitiesGet(fields, 1);

  KjNode* outArray = kjArray(orionldState.kjsonP, "attributeList");

  if (localEntityArray != NULL)
    localAttrNamesExtract(outArray, localEntityArray);

  //
  // GET external attributes - i.e. from the "registrations" collection
  //
  KjNode* remote = dbEntityTypesFromRegistrationsGet(true);

  if (remote)
    remoteAttrNamesExtract(outArray, remote);

  return getEntityAttributesResponse(outArray);
}



// -----------------------------------------------------------------------------
//
// entityDataExtract -
//
bool entityDataExtract(KjNode* _idNodeP, char** entityIdPP, char** entityTypePP)
{
  if (_idNodeP == NULL)
    return false;

  if (_idNodeP->type != KjObject)
    return false;

  KjNode* idNodeP   = kjLookup(_idNodeP, "id");
  KjNode* typeNodeP = kjLookup(_idNodeP, "type");

  if ((idNodeP == NULL) || (typeNodeP == NULL))
    return false;

  if ((idNodeP->type != KjString) || (typeNodeP->type != KjString))
    return false;

  *entityIdPP   = idNodeP->value.s;
  *entityTypePP = orionldContextItemAliasLookup(orionldState.contextP, typeNodeP->value.s, NULL, NULL);

  return true;
}



// -----------------------------------------------------------------------------
//
// attributeInfoAdd -
//
// PARAMETERS
//   arrayAttributeP:     pointer to the attribute info in the output array
//   aP:                  the attribute to be appended to arrayAttributeP
//   entityType:          the type of the entity that aP belongs to
//
//
// 1. Lookup attributeCount and increment
// 2. Lookup attributeTypes and make sure 'attributeType' is present - if not, add
// 3. Lookup typeNames and make sure 'entityType' is present - if not, add
//
static void attributeInfoAdd(KjNode* arrayAttributeP, KjNode* aP, char* entityType)
{
  KjNode*     countP        = kjLookup(arrayAttributeP, "attributeCount");
  KjNode*     attrTypesP    = kjLookup(arrayAttributeP, "attributeTypes");
  KjNode*     typeNamesP    = kjLookup(arrayAttributeP, "typeNames");

  KjNode*     attrTypeNodeP = kjLookup(aP, "type");

  if ((countP == NULL) || (attrTypesP == NULL) || (typeNamesP == NULL))
  {
    LM_E(("Internal Error (missing field: countP:%p, attrTypesP:%p, typeNamesP:%p", countP, attrTypesP, typeNamesP));
    return;
  }

  if (attrTypeNodeP == NULL)
  {
    LM_E(("Internal Error (attribute type not found)"));
    return;
  }
  const char* attributeType = attrTypeNodeP->value.s;

  countP->value.i += 1;

  if (kjStringValueLookupInArray(attrTypesP, attributeType) == NULL)
  {
    KjNode* typeItemP = kjString(orionldState.kjsonP, NULL, attributeType);
    kjChildAdd(attrTypesP, typeItemP);
  }

  if (kjStringValueLookupInArray(typeNamesP, entityType) == NULL)
  {
    KjNode* typeItemP = kjString(orionldState.kjsonP, NULL, entityType);
    kjChildAdd(typeNamesP, typeItemP);
  }
}



// -----------------------------------------------------------------------------
//
// attributeCreate -
//
static KjNode* attributeCreate(KjNode* attrV, KjNode* aP, char* entityType)
{
  KjNode*       attrTypeNodeP    = kjLookup(aP, "type");
  const char*   attributeType    = (attrTypeNodeP != NULL)? attrTypeNodeP->value.s : NULL;
  const char*   attrLongName     = aP->name;
  KjNode*       entityAttrP      = kjObject(orionldState.kjsonP, attrLongName);  // Saving long name of attr as name of the object
  KjNode*       idP              = kjString(orionldState.kjsonP,  "id", attrLongName);
  KjNode*       typeP            = kjString(orionldState.kjsonP,  "type", "Attribute");
  KjNode*       attributeCountP  = kjInteger(orionldState.kjsonP, "attributeCount", 1);
  KjNode*       attributeTypesP  = kjArray(orionldState.kjsonP,   "attributeTypes");
  KjNode*       typeNamesP       = kjArray(orionldState.kjsonP,   "typeNames");
  char*         attrShortName    = orionldContextItemAliasLookup(orionldState.contextP, attrLongName, NULL, NULL);

  if (orionldState.acceptJsonld == true)
  {
    KjNode* contextNode = kjString(orionldState.kjsonP, "@context", orionldState.contextP->url);
    kjChildAdd(entityAttrP, contextNode);
  }

  kjChildAdd(entityAttrP, idP);
  kjChildAdd(entityAttrP, typeP);
  kjChildAdd(entityAttrP, attributeCountP);
  kjChildAdd(entityAttrP, attributeTypesP);
  kjChildAdd(entityAttrP, typeNamesP);

  if (attrShortName != attrLongName)
  {
    KjNode* attributeNameNodeP = kjString(orionldState.kjsonP, "attributeName", attrShortName);

    kjChildAdd(entityAttrP, attributeNameNodeP);
  }

  KjNode* entityTypeP = kjString(orionldState.kjsonP, NULL, entityType);
  kjChildAdd(typeNamesP, entityTypeP);

  if (attributeType != NULL)
  {
    KjNode* attributeTypeP = kjString(orionldState.kjsonP, NULL, attributeType);
    kjChildAdd(attributeTypesP, attributeTypeP);
  }

  // Finally, add the entityAttr to the attribute array
  kjChildAdd(attrV, entityAttrP);

  return entityAttrP;
}



// -----------------------------------------------------------------------------
//
// attributeLookup -
//
static KjNode* attributeLookup(KjNode* attrV, char* attrLongName)
{
  for (KjNode* attrObjectP = attrV->value.firstChildP; attrObjectP != NULL; attrObjectP = attrObjectP->next)
  {
    if (strcmp(attrObjectP->name, attrLongName) == 0)
      return attrObjectP;
  }

  return NULL;
}



// -----------------------------------------------------------------------------
//
// dbEntityAttributesGetWithDetails -
//
// The payload body of the response looks like this:
// [
//   {
//     "id": "urn:...:A1",         # The FQN of the Attribute
//     "type": "Attribute",        # Constant (JSON-LD)
//     "attributeName": "A1",      # short name if found in @context
//     "attributeCount": 23,       # Number of attribute instances with this attribute name
//     "attributeTypes": ["",""]   # List of attribute types of the the attribute in the current entities
//     "typeNames": ["",""]        # Types of entities that has an attribute
//   },
//   { ... }
// ]
//
// The output from dbEntitiesGet looks like this;
// [
//     {
//         "_id": {
//             "id": "urn:ngsi-ld:entities:E0",
//             "servicePath": "/",
//             "type": "https://uri.etsi.org/ngsi-ld/default-context/T"
//         },
//         "attrs": {
//             "https://uri=etsi=org/ngsi-ld/default-context/P1": {
//                 "creDate": 1619103030.209846,
//                 "mdNames": [],
//                 "modDate": 1619103030.209846,
//                 "type": "Property",
//                 "value": 1
//             },
//             "https://uri=etsi=org/ngsi-ld/default-context/P2": {
//                 "creDate": 1619103030.209846,
//                 "mdNames": [],
//                 "modDate": 1619103030.209846,
//                 "type": "Property",
//                 "value": 1
//             },
//             "https://uri=etsi=org/ngsi-ld/default-context/R1": {
//                 "creDate": 1619103030.209846,
//                 "mdNames": [],
//                 "modDate": 1619103030.209846,
//                 "type": "Relationship",
//                 "value": "urn:ngsu-ld:entities:E1"
//             }
//         }
//     }
// ]
//
//
// WARNING
//   This first implementation will perform a single query to get ALL the info in one go -
//   in the future, we will need pagination for this operation and a cursor iterate out more info from the db ...
//
//
// The function is used for two operations:
//
//   1. GET /attributes?details=true
//   2. GET /attributes/{attrName}
//
// Case 2 has a non-NULL 'attributeName' parameter AND:
// - In case of nothing found. NULL shall be returneed (not an empty array)
// - If found, instead of an array, a single Attribute is returned
//
static KjNode* dbEntityAttributesGetWithDetails(OrionldProblemDetails* pdP, char* attributeName)
{
  //
  // GET local attributes - i.e. from the "entities" collection
  //
  KjNode* localEntityArray;
  char*   fields[2] = { (char*) "_id", (char*) "attrs" };

  localEntityArray = dbEntitiesGet(fields, 2);

  if (localEntityArray == NULL)
  {
    if (attributeName == NULL)  // GET /attributes?details=true
    {
      KjNode* empty = kjArray(orionldState.kjsonP, NULL);
      return empty;
    }
    else
    {
      orionldProblemDetailsFill(pdP, OrionldResourceNotFound, "Attribute Not Found", attributeName, 404);
      orionldErrorResponseCreate(pdP->type, pdP->title, pdP->detail);
      return orionldState.responseTree;
    }
  }


  //
  // The attributes are stored in mongo with all dots replaced for '=', so ...
  //
  char* attributeNameEq = NULL;

  if (attributeName != NULL)
  {
    attributeNameEq = kaStrdup(&orionldState.kalloc, attributeName);
    dotForEq(attributeNameEq);
  }

  // Looping over output from dbEntitiesGet
  KjNode* attrV = kjArray(orionldState.kjsonP, NULL);
  for (KjNode* entityP = localEntityArray->value.firstChildP; entityP != NULL; entityP = entityP->next)
  {
    KjNode* _idNode   = kjLookup(entityP, "_id");
    KjNode* attrsNode = kjLookup(entityP, "attrs");
    char*   entityId;
    char*   entityType;

    if (attrsNode == NULL)                     // "attrs" not even there
      continue;
    if (attrsNode->type != KjObject)           // "attrs" is of wrong type - bug in DB!!!
      continue;
    if (attrsNode->value.firstChildP == NULL)  // "attrs" there but empty
      continue;

    if ((_idNode == NULL) || (entityDataExtract(_idNode, &entityId, &entityType) == false))
      continue;

    for (KjNode* aP = attrsNode->value.firstChildP; aP != NULL; aP = aP->next)
    {
      //
      // For GET /attributes/{attributeName}, one single attribute is considered
      //
      if ((attributeNameEq != NULL) && (strcmp(aP->name, attributeNameEq) != 0))
        continue;

      //
      // Lookup the attribute in the current output
      // Not there?  Create it
      //
      eqForDot(aP->name);
      KjNode* arrayAttributeP = attributeLookup(attrV, aP->name);
      if (arrayAttributeP == NULL)
        arrayAttributeP = attributeCreate(attrV, aP, entityType);
      else
        attributeInfoAdd(arrayAttributeP, aP, entityType);
    }
  }

  if (attributeNameEq != NULL)
  {
    if (attrV->value.firstChildP == NULL)
    {
      orionldProblemDetailsFill(pdP, OrionldResourceNotFound, "Attribute Not Found", attributeName, 404);
      orionldErrorResponseCreate(pdP->type, pdP->title, pdP->detail);
      return orionldState.responseTree;
    }

    return attrV->value.firstChildP;
  }

  return attrV;
}



// ----------------------------------------------------------------------------
//
// dbEntityAttributesGet -
//
KjNode* dbEntityAttributesGet(OrionldProblemDetails* pdP, char* attribute, bool details)
{
  bzero(pdP, sizeof(OrionldProblemDetails));

  if (details == false)
    return dbEntityAttributesGetWithoutDetails(pdP);
  else
    return dbEntityAttributesGetWithDetails(pdP, attribute);
}
