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
#include "orionld/context/orionldContextItemAliasLookup.h"        // orionldContextItemAliasLookup
#include "orionld/db/dbConfiguration.h"                           // dbEntityAttributesFromRegistrationsGet, dbEntitiesGet
#include "orionld/db/dbEntityAttributesGet.h"                     // Own interface


extern KjNode* kjArrayStringLookup(KjNode* stringArray, const char* str);   // FIXME: move to kjson lib
extern void    kjStringArraySortedInsert(KjNode* array, KjNode* newItemP);  // FIXME: move to kjson lib



// -----------------------------------------------------------------------------
//
// getEntityAttributesResponse - All entity attributes for which entity instances are currently available in the NGSI-LD system.
//
static KjNode* getEntityAttributesResponse(KjNode* sortedArrayP)
{
  char entityAttributesId[128];

  strncpy(entityAttributesId, "urn:ngsi-ld:EntityAttributeList:", sizeof(entityAttributesId));
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
// attrNamesExtract -
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
static void attrNamesExtract(KjNode* outArray, KjNode* local)
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

      if (kjArrayStringLookup(outArray, aP->value.s) == NULL)
      {
        kjChildRemove(objP, aP);
        kjStringArraySortedInsert(outArray, aP);
      }

      aP = next;
    }
  }
}



// -----------------------------------------------------------------------------
//
// dbEntityAttributesGetWithoutDetails -
//
static KjNode* dbEntityAttributesGetWithoutDetails(OrionldProblemDetails* pdP)
{
  KjNode* localEntityArray;
  char*   fields[1] = { (char*) "attrNames" };

  //
  // GET local attributes - i.e. from the "entities" collection
  //
  localEntityArray = dbEntitiesGet(fields, 1);

  KjNode* outArray = kjArray(orionldState.kjsonP, "attributeList");

  if (localEntityArray != NULL)
    attrNamesExtract(outArray, localEntityArray);
  else
  {
  }

  return getEntityAttributesResponse(outArray);
}



// -----------------------------------------------------------------------------
//
// dbEntityAttributesGetWithDetails -
//
static KjNode* dbEntityAttributesGetWithDetails(OrionldProblemDetails* pdP)
{
  orionldProblemDetailsFill(pdP, OrionldOperationNotSupported, "Not Implemented", "GET /ngsi-ld/v1/attributes?details=true", 501);
  orionldErrorResponseCreate(OrionldOperationNotSupported, pdP->title, pdP->detail);
  orionldState.noLinkHeader   = true;  // We don't want the Link header for non-implemented requests

  return orionldState.responseTree;
}



// ----------------------------------------------------------------------------
//
// dbEntityAttributesGet -
//
KjNode* dbEntityAttributesGet(OrionldProblemDetails* pdP)
{
  if (orionldState.uriParams.details == false)
    return dbEntityAttributesGetWithoutDetails(pdP);
  else
    return dbEntityAttributesGetWithDetails(pdP);
}
