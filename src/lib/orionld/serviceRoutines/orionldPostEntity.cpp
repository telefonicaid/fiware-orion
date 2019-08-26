/*
*
* Copyright 2018 Telefonica Investigacion y Desarrollo, S.A.U
*
* This file is part of Orion Context Broker.
*
* Orion Context Broker is free software: you can redistribute it and/or
* modify it under the terms of the GNU Affero General Public License as
* published by the Free Software Foundation, either version 3 of the
* License, or (at your option) any later version.
*
* Orion Context Broker is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero
* General Public License for more details.
*
* You should have received a copy of the GNU Affero General Public License
* along with Orion Context Broker. If not, see http://www.gnu.org/licenses/.
*
* For those usages not covered by this license please contact with
* iot_support at tid dot es
*
* Author: Ken Zangelin
*/
#include <string.h>                                              // strlen

extern "C"
{
#include "kjson/kjBuilder.h"                                     // kjChildRemove
#include "kjson/kjRender.h"                                      // kjRender
#include "kjson/kjLookup.h"                                      // kjLookup
#include "kalloc/kaAlloc.h"                                      // kaAlloc
}

#include "logMsg/logMsg.h"                                       // LM_*
#include "logMsg/traceLevels.h"                                  // Lmt*

#include "mongoBackend/mongoUpdateContext.h"                     // mongoUpdateContext
#include "mongoBackend/mongoEntityExists.h"                      // mongoEntityExists
#include "ngsi10/UpdateContextRequest.h"                         // UpdateContextRequest
#include "ngsi10/UpdateContextResponse.h"                        // UpdateContextResponse
#include "rest/ConnectionInfo.h"                                 // ConnectionInfo
#include "orionld/common/CHECK.h"                                // CHECK
#include "orionld/common/SCOMPARE.h"                             // SCOMPAREx
#include "orionld/common/orionldErrorResponse.h"                 // orionldErrorResponseCreate
#include "orionld/common/orionldState.h"                         // orionldState
#include "orionld/common/orionldAttributeTreat.h"                // orionldAttributeTreat
#include "orionld/db/dbEntityLookup.h"                           // dbEntityLookup
#include "orionld/db/dbEntityUpdate.h"                           // dbEntityUpdate
#include "orionld/context/orionldContextTreat.h"                 // orionldContextTreat
#include "orionld/context/orionldUriExpand.h"                    // orionldUriExpand
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



// -----------------------------------------------------------------------------
//
// kjTreeToContextElement -
//
bool kjTreeToContextElement(ConnectionInfo* ciP, KjNode* treeP, ContextElement* ceP)
{
  // Iterate over the object, to get all attributes
  for (KjNode* kNodeP = orionldState.requestTree->value.firstChildP; kNodeP != NULL; kNodeP = kNodeP->next)
  {
    KjNode*           attrTypeNodeP = NULL;
    ContextAttribute* caP           = new ContextAttribute();

    if (orionldAttributeTreat(ciP, kNodeP, caP, &attrTypeNodeP) == false)
    {
      LM_E(("orionldAttributeTreat failed"));
      delete caP;
      return false;
    }

    //
    // URI Expansion for the attribute name, except if "location", "observationSpace", or "operationSpace"
    //
    if (SCOMPARE9(kNodeP->name,       'l', 'o', 'c', 'a', 't', 'i', 'o', 'n', 0))
      caP->name = kNodeP->name;
    else if (SCOMPARE17(kNodeP->name, 'o', 'b', 's', 'e', 'r', 'v', 'a', 't', 'i', 'o', 'n', 'S', 'p', 'a', 'c', 'e', 0))
      caP->name = kNodeP->name;
    else if (SCOMPARE15(kNodeP->name, 'o', 'p', 'e', 'r', 'a', 't', 'i', 'o', 'n', 'S', 'p', 'a', 'c', 'e', 0))
      caP->name = kNodeP->name;
    else
    {
      char  longName[256];
      char* details;

      if (orionldUriExpand(orionldState.contextP, kNodeP->name, longName, sizeof(longName), &details) == false)
      {
        delete caP;
        orionldErrorResponseCreate(OrionldBadRequestData, details, kNodeP->name, OrionldDetailsAttribute);
        return false;
      }

      caP->name = longName;
    }

    // NO URI Expansion for Attribute TYPE
    caP->type = attrTypeNodeP->value.s;

    // Add the attribute to the attr vector
    ceP->contextAttributeVector.push_back(caP);
  }

  return true;
}



// -----------------------------------------------------------------------------
//
// kjTreeMergeAddNewAttrsIgnoreExisting -
//
bool kjTreeMergeAddNewAttrsIgnoreExisting(KjNode* sourceTree, KjNode* modTree)
{
  for (KjNode* modAttrP = modTree->value.firstChildP; modAttrP != NULL; modAttrP = modAttrP->next)
  {
    //
    // If modAttrP exists in sourceTree, then we ignore the attr and it stays in modTree
    //
    if (kjLookup(sourceTree, modAttrP->name) != NULL)
      continue;

    // Not there - remove modAttrP from modTree and add to sourceTree
    kjChildRemove(modTree, modAttrP);
    kjChildAdd(sourceTree, modAttrP);
  }

  return true;
}



// -----------------------------------------------------------------------------
//
// kjNodeAttributeMerge
//
bool kjNodeAttributeMerge(KjNode* sourceP, KjNode* updateP)
{
  //
  // Go over the entire updateP tree and replace all those nodes in sourceP
  // Also, update the modDate node to the current date/time
  //
  KjNode* modDateP = kjLookup(sourceP, "modDate");

  modDateP->value.i = time(NULL);

  int ix = 0;
  KjNode* nodeP = updateP->value.firstChildP;

  while (nodeP != NULL)
  {
    KjNode* next = nodeP->next;

    LM_TMP(("MERGE: Checking item %d of updateP: %s (next at %p)", ix, nodeP->name, nodeP->next));
    KjNode* sameNodeInSourceP = kjLookup(sourceP, nodeP->name);

    if (sameNodeInSourceP != NULL)
    {
      LM_TMP(("MERGE: Found '%s' member in source - removing it", sameNodeInSourceP->name));
      kjChildRemove(sourceP, sameNodeInSourceP);
    }

    LM_TMP(("MERGE: Adding '%s' member to source", nodeP->name));
    kjChildAdd(sourceP, nodeP);
    ++ix;
    LM_TMP(("MERGE: Treated item %d of updateP: %s (next at %p)", ix, nodeP->name, nodeP->next));

    nodeP = next;
  }

  return true;
}



// -----------------------------------------------------------------------------
//
// kjSysAttrs - add sysAttrs (creDate + modDate) to an attribute
//
void kjSysAttrs(KjNode* attrP)
{
  KjNode* creDateP = NULL;
  KjNode* modDateP = NULL;

  for (KjNode* nodeP = attrP->value.firstChildP; nodeP != NULL; nodeP = nodeP->next)
  {
    if (SCOMPARE8(nodeP->name, 'm', 'o', 'd', 'D', 'a', 't', 'e', 0))
      modDateP = nodeP;
    else if (SCOMPARE8(nodeP->name, 'c', 'r', 'e', 'D', 'a', 't', 'e', 0))
      creDateP = nodeP;
  }

  if (creDateP == NULL)
  {
    creDateP = kjInteger(orionldState.kjsonP, "creDate", time(NULL));
    kjChildAdd(attrP, creDateP);
  }

  if (modDateP != NULL)
  {
    modDateP->value.i = time(NULL);
    modDateP->type    = KjInt;
  }
  else
  {
    modDateP = kjInteger(orionldState.kjsonP, "modDate", time(NULL));
    kjChildAdd(attrP, modDateP);
  }
}



// -----------------------------------------------------------------------------
//
// kjModDateSet -
//
void kjModDateSet(KjNode* attrP)
{
  for (KjNode* nodeP = attrP->value.firstChildP; nodeP != NULL; nodeP = nodeP->next)
  {
    if (SCOMPARE8(nodeP->name, 'm', 'o', 'd', 'D', 'a', 't', 'e', 0))
    {
      nodeP->value.i = time(NULL);
      return;
    }
  }
}



// -----------------------------------------------------------------------------
//
// objectToValue -
//
static void objectToValue(KjNode* attrP)
{
  if (attrP->type != KjObject)
    return;

  for (KjNode* itemP = attrP->value.firstChildP; itemP != NULL; itemP = itemP->next)
  {
    if (strcmp(itemP->name, "object") == 0)
      itemP->name = (char*) "value";
  }
}



// -----------------------------------------------------------------------------
//
// kjTreeMergeAddNewAttrsOverwriteExisting -
//
bool kjTreeMergeAddNewAttrsOverwriteExisting(KjNode* sourceTree, KjNode* modTree, char** titleP, char** detailsP)
{
  //
  // The data model of Orion is that all attributes go in toplevel::attrs
  // So, we need to reposition "sourceTree" so that it points to the sourceTree::atts
  //
  KjNode* attrNamesP = kjLookup(sourceTree, (char*) "attrNames");
  KjNode* attrsP     = kjLookup(sourceTree, (char*) "attrs");

  if (attrsP == NULL)
  {
    *titleP   = (char*) "Corrupt Database";
    *detailsP = (char*) "No /attrs/ member found in Entity DB data";
    return NULL;
  }

  if (attrNamesP == NULL)
  {
    *titleP   = (char*) "Corrupt Database";
    *detailsP = (char*) "No /attrNamess/ member found in Entity DB data";
    return NULL;
  }

  KjNode* modAttrP = modTree->value.firstChildP;

  while (modAttrP != NULL)
  {
    KjNode* next = modAttrP->next;

    // If the attribute is a Relationship, then the "object" field should change name to "value"
    objectToValue(modAttrP);

    //
    // If "modAttrP" exists in sourceTree, then we have to remove it - to later add the one from "modTree"
    //   - if found in sourceTree: merge sourceTreeAttrP with modAttrP
    //   - if NOT found:
    //     - remove modAttrP from modTree
    //     - add modAttrP to sourceTree
    //     - add slot in "attrNames"
    //
    KjNode* sourceTreeAttrP = NULL;
    if ((sourceTreeAttrP = kjLookup(attrsP, modAttrP->name)) != NULL)
    {
#if 0
      char renderBuffer[1024];

      LM_TMP(("MERGE: Found attribute '%s' is source tree - merging with the new one", modAttrP->name));
      kjRender(orionldState.kjsonP, sourceTreeAttrP, renderBuffer, sizeof(renderBuffer));
      LM_TMP(("MERGE: sourceTreeAttrP: '%s'", renderBuffer));
      kjRender(orionldState.kjsonP, modAttrP, renderBuffer, sizeof(renderBuffer));
      LM_TMP(("MERGE: modAttrP: '%s'", renderBuffer));

      LM_TMP(("MERGE: calling kjNodeAttributeMerge"));
#endif
      kjNodeAttributeMerge(sourceTreeAttrP, modAttrP);
      kjModDateSet(sourceTreeAttrP);
    }
    else
    {
      LM_TMP(("MERGE: Did not find attribute '%s' in source tree - adding the new one", modAttrP->name));


      // Remove modAttrP from modTree and add to sourceTree
      LM_TMP(("MERGE: Remove modAttrP '%s' from modTree and add to sourceTree", modAttrP->name));
      kjChildRemove(modTree, modAttrP);
      kjChildAdd(attrsP, modAttrP);
      kjSysAttrs(modAttrP);

      // Orion Data Model: Must add a mdNames: []
      KjNode* mdArrayP = kjArray(orionldState.kjsonP, "mdNames");
      kjChildAdd(modAttrP, mdArrayP);

      //
      // Add attribute name to "attrNames"
      //
      KjNode* attrName = kjString(orionldState.kjsonP, "", modAttrP->name);

      //
      // The dots in the attribute name have been replaced with '='
      // We need to change that back before adding to "attrNames"
      //
      char* cP = attrName->value.s;

      while (*cP != 0)
      {
        if (*cP == '=')
          *cP = '.';
        ++cP;
      }
      kjChildAdd(attrNamesP, attrName);
    }

    modAttrP = next;
  }

  return true;
}



// -----------------------------------------------------------------------------
//
// expandAttrNames -
//
static bool expandAttrNames(KjNode* treeP, char** detailsP)
{
  for (KjNode* attrP = treeP->value.firstChildP; attrP != NULL; attrP = attrP->next)
  {
    char expanded[512];

    if (orionldUriExpand(orionldState.contextP, attrP->name, expanded, sizeof(expanded), detailsP) == false)
      return false;

    int sLen = strlen(expanded);

    if ((attrP->name = kaAlloc(&orionldState.kalloc, sLen + 1)) == NULL)
    {
      *detailsP = (char*) "out of memory";
      return false;
    }

    strcpy(attrP->name, expanded);

    //
    // Any '.' must be converted into a '='
    //
    char* nameP = attrP->name;
    while (*nameP != 0)
    {
      if (*nameP == '.')
        *nameP = '=';
      ++nameP;
    }
  }

  return true;
}



// -----------------------------------------------------------------------------
//
// orionldPostEntityOverwrite -
//
bool orionldPostEntityOverwrite(ConnectionInfo* ciP)
{
  LM_TMP(("In orionldPostEntityOverwrite"));
  //
  // Forwarding and Subscriptions will be taken care of later.
  // For now, just local updates
  //
  // 1. Get entity, as a KjNode tree
  // 2. For each attribute in orionldState.requestTree:
  //    - If found in currentEntityTreeP, merge both
  //    - If not found - add attribute from orionldState.requestTree to currentEntityTreeP (also add to "attrNames[]")
  // 3. Write to mongo
  //
  char*   entityId           = orionldState.wildcard[0];
  LM_TMP(("-------------- Calling dbEntityLookup(%s)", entityId));
  KjNode* currentEntityTreeP = dbEntityLookup(entityId);
  LM_TMP(("-------------- After dbEntityLookup(%s): %p", entityId, currentEntityTreeP));
  char*   title;
  char*   details;

  if (currentEntityTreeP == NULL)
  {
    ciP->httpStatusCode = SccNotFound;
    orionldErrorResponseCreate(OrionldBadRequestData, "Entity does not exist", orionldState.wildcard[0], OrionldDetailsString);
    return false;
  }

#if 0
  char renderBuffer[1024];
  kjRender(orionldState.kjsonP, currentEntityTreeP, renderBuffer, sizeof(renderBuffer));
  LM_TMP(("MERGE: Got the entity: '%s'", renderBuffer));

  kjRender(orionldState.kjsonP, orionldState.requestTree, renderBuffer, sizeof(renderBuffer));
  LM_TMP(("MERGE: Update/noOverwrite: '%s'", renderBuffer));
#endif

  // Expand attribute names
  if (expandAttrNames(orionldState.requestTree, &details) == false)
  {
    ciP->httpStatusCode = SccReceiverInternalError;
    orionldErrorResponseCreate(OrionldBadRequestData, "Can't expand attribute names", details, OrionldDetailsString);
    return false;
  }

  // Merge orionldState.requestTree with currentEntityTreeP
  if (kjTreeMergeAddNewAttrsOverwriteExisting(currentEntityTreeP, orionldState.requestTree, &title, &details) == false)
  {
    ciP->httpStatusCode = SccReceiverInternalError;
    orionldErrorResponseCreate(OrionldInternalError, title, details, OrionldDetailsString);
    return false;
  }

#if 0
  kjRender(orionldState.kjsonP, currentEntityTreeP, renderBuffer, sizeof(renderBuffer));
  LM_TMP(("MERGE: resulting tree: '%s'", renderBuffer));
#endif

  //
  // Setting the modification date of the entity
  //
  kjModDateSet(currentEntityTreeP);

  // Write to database
#if 0
  char buffer[1024];
  kjRender(orionldState.kjsonP, currentEntityTreeP, buffer, sizeof(buffer));
  LM_TMP(("MERGE: writing to DB: %s", buffer));
#endif
  dbEntityUpdate(entityId, currentEntityTreeP);

  //
  // All OK - set HTTP STatus Code
  //
  ciP->httpStatusCode = SccNoContent;

  return true;
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
// Instead we will REIMPLEMENT the whole DB-part, using the newest C driver for mongodb
//
bool orionldPostEntity(ConnectionInfo* ciP)
{
  LM_TMP(("In orionldPostEntity"));

  // Is the payload not a JSON object?
  OBJECT_CHECK(orionldState.requestTree, kjValueType(orionldState.requestTree->type));

  if (orionldState.uriParamOptions.noOverwrite == false)
    return orionldPostEntityOverwrite(ciP);

  // 1. Check that the entity exists
  if (mongoEntityExists(orionldState.wildcard[0], orionldState.tenant) == false)
  {
    ciP->httpStatusCode = SccNotFound;
    orionldErrorResponseCreate(OrionldBadRequestData, "Entity does not exist", orionldState.wildcard[0], OrionldDetailsString);
    return false;
  }

  UpdateContextRequest   mongoRequest;
  UpdateContextResponse  mongoResponse;
  ContextElement*        ceP       = new ContextElement();  // FIXME: Any way I can avoid to allocate ?
  EntityId*              entityIdP;

  mongoRequest.updateActionType = ActionTypeAppendStrict;

  mongoRequest.contextElementVector.push_back(ceP);
  entityIdP     = &mongoRequest.contextElementVector[0]->entityId;
  entityIdP->id = orionldState.wildcard[0];



  if (kjTreeToContextElement(ciP, orionldState.requestTree, ceP) == false)
  {
    // kjTreeToContextElement fills in error using 'orionldErrorResponseCreate()'
    mongoRequest.release();
    LM_E(("kjTreeToContextElement failed"));
    return false;
  }

  //
  // This service may return a payload, to indicate which attributes have been appended/changed
  // Also, its returned HTTP Status Code depends on the appended/changed attributes.
  // So. we need mongoBackend to give us that information back, in a way that can easily be used.
  // A simple string will be used, with the following design:
  //
  //   "|attrName|attrName|attrName|attrName|"
  //
  // HTTP Status Codes:
  //  * 204: All attributes in the payload were successfully appended (or overwritten)
  //  * 207: Only the Attributes included in the response payload were successfully appended.
  //  * 400: The request or its content is incorrect
  //  * 404: Entity not found
  //
  // In the case "207" - some of the attributes weren't successfully updated, the list of all
  // successfully updated attributes in returned as payload.
  // This list is a vector of attribute names.
  //
  // For better performance, the names of the erroneous attributes are recored in a string.
  // After "mongoBackend processing" this erroneous attributes string is checked and if it is not empty,
  // then an inverted array must be created for the response payload.
  // Normally, all will work just fine so this array will be empty.
  //
  // The variable to hold this "error attribute array" is:
  //   orionldState.errorAttributeArray.
  //
  // The function to use to insert the attributes is:
  //   orionldStateErrorAttributeAdd(char* attributeName)
  //


  //
  // Call mongoBackend
  //
  ciP->httpStatusCode = mongoUpdateContext(&mongoRequest,
                                           &mongoResponse,
                                           orionldState.tenant,
                                           ciP->servicePathV,
                                           ciP->uriParam,
                                           ciP->httpHeaders.xauthToken,
                                           ciP->httpHeaders.correlator,
                                           ciP->httpHeaders.ngsiv2AttrsFormat,
                                           ciP->apiVersion,
                                           NGSIV2_NO_FLAVOUR);

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
      orionldErrorResponseCreate(OrionldBadRequestData, "Internal Error", "Error from Mongo-DB Backend", OrionldDetailsString);
    }

    retValue = false;
  }

  mongoRequest.release();
  mongoResponse.release();

  return retValue;
}
