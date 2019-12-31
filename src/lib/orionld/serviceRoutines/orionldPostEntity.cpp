/*
*
* Copyright 2018 FIWARE Foundation e.V.
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
#include <string.h>                                              // strlen
#include <string>                                                // std::string
#include <vector>                                                // std::vector

extern "C"
{
#include "kbase/kMacros.h"                                       // K_VEC_SIZE, K_FT
#include "kjson/kjBuilder.h"                                     // kjChildRemove
#include "kjson/kjRender.h"                                      // kjRender
#include "kjson/kjLookup.h"                                      // kjLookup
#include "kjson/kjClone.h"                                       // kjClone
#include "kalloc/kaAlloc.h"                                      // kaAlloc
#include "kalloc/kaStrdup.h"                                     // kaStrdup
}

#include "logMsg/logMsg.h"                                       // LM_*
#include "logMsg/traceLevels.h"                                  // Lmt*

#include "common/MimeType.h"                                     // MimeType
#include "mongoBackend/mongoUpdateContext.h"                     // mongoUpdateContext
#include "ngsi10/UpdateContextRequest.h"                         // UpdateContextRequest
#include "ngsi10/UpdateContextResponse.h"                        // UpdateContextResponse
#include "rest/ConnectionInfo.h"                                 // ConnectionInfo
#include "ngsi/ParseData.h"                                      // ParseData
#include "serviceRoutines/postUpdateContext.h"                   // postUpdateContext

#include "orionld/common/CHECK.h"                                // CHECK
#include "orionld/common/SCOMPARE.h"                             // SCOMPAREx
#include "orionld/common/orionldErrorResponse.h"                 // orionldErrorResponseCreate
#include "orionld/common/orionldState.h"                         // orionldState
#include "orionld/common/dotForEq.h"                             // dotForEq
#include "orionld/common/eqForDot.h"                             // eqForDot
#include "orionld/db/dbEntityLookup.h"                           // dbEntityLookup
#include "orionld/db/dbEntityUpdate.h"                           // dbEntityUpdate
#include "orionld/context/orionldContextItemExpand.h"            // orionldContextItemExpand
#include "orionld/context/orionldContextValueExpand.h"           // orionldContextValueExpand
#include "orionld/context/orionldContextItemAliasLookup.h"       // orionldContextItemAliasLookup
#include "orionld/kjTree/kjTreeToEntity.h"                       // kjTreeToEntity
#include "orionld/serviceRoutines/orionldPostEntity.h"           // Own Interface



// ----------------------------------------------------------------------------
//
// orionldPartialUpdateResponseCreate -
//
void orionldPartialUpdateResponseCreate(ConnectionInfo* ciP)
{
  //
  // Rob the incoming Request Tree - performance to be won!
  //
  orionldState.responseTree = orionldState.requestTree;
  orionldState.requestTree  = NULL;

  //
  // For all attrs in orionldState.responseTree, remove those that are found in orionldState.errorAttributeArrayP.
  // Remember, the format of orionldState.errorAttributeArrayP is:
  //
  //   |attrName|attrName|attrName|...
  //

  KjNode* attrNodeP = orionldState.responseTree->value.firstChildP;

  while (attrNodeP != NULL)
  {
    char*   match;
    KjNode* next = attrNodeP->next;
    bool    moved = false;

    if ((match = strstr(orionldState.errorAttributeArrayP, attrNodeP->name)) != NULL)
    {
      if ((match[-1] == '|') && (match[strlen(attrNodeP->name)] == '|'))
      {
        kjChildRemove(orionldState.responseTree, attrNodeP);
        attrNodeP = next;
        moved = true;
      }
    }

    if (moved == false)
      attrNodeP = attrNodeP->next;
  }
}



// ----------------------------------------------------------------------------
//
// attrToDb -
//
void attrToDb(KjNode* dbAttrsP, KjNode* dbAttrNamesP, KjNode* attrP)
{
  kjChildRemove(orionldState.requestTree, attrP);
  kjChildAdd(dbAttrsP, attrP);

  //
  // Add attribute name to "attrNames", but without '=' instead of '.'
  // BUT - only if necessary.
  // If an attribute is OVERWRITTEN, then it already exists in "attrNames"
  //
  if (dbAttrNamesP != NULL)
  {
    char    name[256];
    strcpy(name, attrP->name);
    eqForDot(name);
    KjNode* nameP = kjString(orionldState.kjsonP, NULL, name);

    kjChildAdd(dbAttrNamesP, nameP);
  }
}



// -----------------------------------------------------------------------------
//
// kjTreePresent - short presentation of a KjNode tree that is an Entity
//
// This function is aware of:
// - "attrs"
// - "attrNames"
// - "md"
// - "mdNames"
//
void kjTreePresent(const char* prefix, const char* what, KjNode* tree)
{
  LM_TMP(("%s: ---------------- %s -------------------", prefix, what));
  for (KjNode* eNodeP = tree->value.firstChildP; eNodeP != NULL; eNodeP = eNodeP->next)
  {
    if (strcmp(eNodeP->name, "_id") == 0)
    {
      for (KjNode* idNodeP = eNodeP->value.firstChildP; idNodeP != NULL; idNodeP = idNodeP->next)
      {
        if (strcmp(idNodeP->name, "id") == 0)
          LM_TMP(("%s: Entity ID: '%s'", prefix, idNodeP->value.s));
        else if (strcmp(idNodeP->name, "type") == 0)
          LM_TMP(("%s: Entity TYPE: '%s'", prefix, idNodeP->value.s));
      }
    }
    else if (strcmp(eNodeP->name, "attrNames") == 0)
    {
      for (KjNode* attrNodeP = eNodeP->value.firstChildP; attrNodeP != NULL; attrNodeP = attrNodeP->next)
      {
        LM_TMP(("%s: AttrName: '%s'", prefix, attrNodeP->value.s));
      }
    }
    else if (strcmp(eNodeP->name, "attrs") == 0)
    {
      for (KjNode* attrNodeP = eNodeP->value.firstChildP; attrNodeP != NULL; attrNodeP = attrNodeP->next)
      {
        LM_TMP(("%s: Attr: '%s'", prefix, attrNodeP->name));
      }
    }
    else
      LM_TMP(("%s: Other: '%s'", prefix, eNodeP->name));
  }
  LM_TMP(("%s: -----------------------------------------------------------", prefix));
}



// ----------------------------------------------------------------------------
//
// orionldPostEntity -
//
// POST /ngsi-ld/v1/entities/*/attrs
//
// URI PARAMETERS
//   options=noOverwrite
//
// From ETSI spec:
//   Behaviour
//   * If the Entity Id is not present or it is not a valid URI then an error of type BadRequestData shall be raised.
//   * If the NGSI-LD endpoint does not know about this Entity, because there is no an existing Entity which id
//     (URI) is equivalent to the one passed as parameter, an error of type ResourceNotFound shall be raised.
//   * For each Attribute (Property or Relationship) included by the Entity Fragment at root level:
//     * If the target Entity does not include a matching Attribute then such Attribute shall be appended to the target Entity
//     * If the target Entity already includes a matching Attribute
//       - If no datasetId is present in the Attribute included by the Entity Fragment:
//         * If options==noOverwrite: the existing Attribute in the target Entity shall be left untouched
//         * If options!=noOverwrite: the existing Attribute in the target Entity shall be replaced by the new one supplied.
//
//
// If options=noOverwrite is set, then we can simply use updateActionType == ActionTypeAppendStrict
//
// If options=noOverwrite is NOT set, then we have no matching already existing function.
// Tries have been made to modify mongoBackend but without success - see issue https://github.com/FIWARE/context.Orion-LD/issues/153
//
// INSTEAD of using ActionTypeAppendStrict for options=noOverwrite and trying to make the non-noOverwrite case work we will do the following:
//
// 1. Get the entity from mongo - as a KjNode tree (db + mongoCppLegacy lib)
// 2. Merge entity from mongo with attrs from DB
//    - If OK to overwrite, clashing attrs will be removed from 'entity from mongo'
//    - If NOT OK to overwrite, clashing attrs will be removed from 'attrs from payload'
//    - New attributes will be simply added to 'entity from mongo'
//    - Make sure creDate is preserved
//    - Make sure modDate is updated
// 3. Create UpdateContext from the KjNode tree 'entity from mongo'
// 4. Call postUpdateContext to REPLACE the "new" entity ('entity from mongo') - make sure creDate is preserved
//
// Like this, we get subscriptions and forwarding for free and the operation works as the spec says.
// We use mongoBackend only in the last step.
//
bool orionldPostEntity(ConnectionInfo* ciP)
{
  // Is the payload not a JSON object?
  OBJECT_CHECK(orionldState.requestTree, kjValueType(orionldState.requestTree->type));

  // 1. Get the entity from mongo
  KjNode* entityFromDb;
  if ((entityFromDb = dbEntityLookup(orionldState.wildcard[0])) == NULL)
  {
    ciP->httpStatusCode = SccNotFound;
    orionldErrorResponseCreate(OrionldBadRequestData, "Entity does not exist", orionldState.wildcard[0]);
    return false;
  }

  // kjTreePresent("MKT", "DB-Tree IN", entityFromDb);

  //
  // 2. Merge entity from mongo with attrs from DB
  //    - If OK to overwrite, clashing attrs will be removed from 'entity from mongo'
  //    - If NOT OK to overwrite, clashing attrs will be removed from 'attrs from payload'
  //    - New attributes will be simply added to 'entity from mongo'
  //    - Make sure creDate is preserved
  //    - Make sure modDate is updated
  //
  KjNode* dbAttrNamesP = kjLookup(entityFromDb, "attrNames");
  KjNode* dbAttrsP     = kjLookup(entityFromDb, "attrs");

  if ((dbAttrNamesP == NULL) || (dbAttrsP == NULL))
  {
    LM_E(("Database Error (attrs/attrNames not found as part of Entity in database)"));
    orionldErrorResponseCreate(OrionldInternalError, "Database Error", "attrs/attrNames not found as part of Entity in database");
    ciP->httpStatusCode = SccReceiverInternalError;
    return false;
  }

  // kjTreePresent("MKT", "Payload-Tree IN", orionldState.requestTree);

  // For all attrs in incoming payload
  KjNode* attrP = orionldState.requestTree->value.firstChildP;
  KjNode* next;
  int     modifiedAttrs =  0;

  while (attrP != NULL)
  {
    KjNode* dbAttrP;
    bool    valueMayBeExpanded = false;

    attrP->name = orionldContextItemExpand(orionldState.contextP, attrP->name, &valueMayBeExpanded, true, NULL);

    if (valueMayBeExpanded == true)
      orionldContextValueExpand(attrP);

    next = attrP->next;
    dotForEq(attrP->name);
    dbAttrP = kjLookup(dbAttrsP, attrP->name);

    if (dbAttrP == NULL)  // Not found in DB - add to dbAttrsP
    {
      attrToDb(dbAttrsP, dbAttrNamesP, attrP);
      ++modifiedAttrs;
    }
    else
    {
      // If OK to overwrite - remove from dbAttrsP, then add payload-attr to db-attr-list
      if (orionldState.uriParamOptions.noOverwrite == false)
      {
        kjChildRemove(dbAttrsP, dbAttrP);
        attrToDb(dbAttrsP, NULL, attrP);  // attrP is removed from its container - so 'while', not 'for (KjNode* attrP = ...)'
        ++modifiedAttrs;
      }
      // else do nothing
    }

    attrP = next;
  }

  if (modifiedAttrs == 0)
  {
    ciP->httpStatusCode = SccNoContent;
    return true;
  }

  //
  // Due to the data model, all '.' in attribute names have been replaced by '=' (to make 'q' work)
  // Before returning the entity, the '=' should be put back to '.'
  //
  KjNode* attrsP = kjLookup(entityFromDb, "attrs");
  if (attrsP != NULL)
  {
    for (KjNode* attrP = attrsP->value.firstChildP; attrP != NULL; attrP = attrP->next)
    {
      eqForDot(attrP->name);
    }
  }

  // kjTreePresent("MKT", "Merged-Tree IN", entityFromDb);

  // 3. Create UpdateContext from the KjNode tree 'entity from mongo'
  ParseData        parseData;
  ContextElement*  ceP = new ContextElement();
  std::string      s;

  parseData.upcr.res.contextElementVector.push_back(ceP);
  kjTreeToEntity(&parseData.upcr.res, entityFromDb);

  parseData.upcr.res.updateActionType = ActionTypeReplace;

#if 0
  //
  // <DEBUG>
  //
  ceP = parseData.upcr.res.contextElementVector[0];
  LM_TMP(("MKT: Entity Id:   %s", ceP->entityId.id.c_str()));
  LM_TMP(("MKT: Entity Type: %s", ceP->entityId.type.c_str()));
  LM_TMP(("MKT: %d Attributes", ceP->contextAttributeVector.size()));
  for (unsigned int ix = 0; ix < ceP->contextAttributeVector.size(); ix++)
    LM_TMP(("MKT: Attributes %d: %s", ix, ceP->contextAttributeVector[ix]->name.c_str()));

  for (unsigned int ix = 0; ix < ceP->contextAttributeVector.size(); ix++)
  {
    ContextAttribute* aP = ceP->contextAttributeVector[ix];

    LM_TMP(("MKT: Attribute %d. Name: %s", ix, aP->name.c_str()));
    LM_TMP(("MKT: Attribute %d. Type: %s", ix, aP->type.c_str()));
    LM_TMP(("MKT: Attribute %d. Value of type %s", ix, valueTypeName(aP->valueType)));
    LM_TMP(("MKT: Attribute %d. %d sub-attrs", ix, aP->metadataVector.size()));
    for (unsigned int mdIx = 0; mdIx < aP->metadataVector.size(); mdIx++)
    {
      Metadata* mdP = aP->metadataVector[mdIx];

      LM_TMP(("MKT:   sub-attr %d. Name: %s", mdIx, mdP->name.c_str()));
      LM_TMP(("MKT:   sub-attr %d. Type: %s", mdIx, mdP->type.c_str()));
      LM_TMP(("MKT:   sub-attr %d. Value of type %s", mdIx, valueTypeName(mdP->valueType)));
    }
  }
#endif


  // 4. Call postUpdateContext to REPLACE the "new" entity ('entity from mongo') - make sure creDate is preserved
  std::vector<std::string> nada;
  s = postUpdateContext(ciP, 0, nada, &parseData, NGSIV2_NO_FLAVOUR);

  //
  // Now check orionldState.errorAttributeArray to see whether any attribute failed to be updated
  //
  bool partialUpdate = (orionldState.errorAttributeArrayP[0] == 0)? false : true;
  bool retValue      = true;

  if (ciP->httpStatusCode == SccOk)
    ciP->httpStatusCode = SccNoContent;
  else
  {
    if (partialUpdate == true)
    {
      orionldPartialUpdateResponseCreate(ciP);
      ciP->httpStatusCode = (HttpStatusCode) 207;
    }
    else
    {
      LM_E(("mongoUpdateContext: HTTP Status Code: %d", ciP->httpStatusCode));
      orionldErrorResponseCreate(OrionldBadRequestData, "Internal Error", "Error from Mongo-DB Backend");
    }

    retValue = false;
  }

  if (ciP->httpStatusCode == (HttpStatusCode) 204)
    orionldState.responseTree = NULL;

  // delete ceP;
  return retValue;
}
