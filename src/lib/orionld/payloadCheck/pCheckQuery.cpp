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
#include "kalloc/kaAlloc.h"                                      // kaAlloc
#include "kjson/KjNode.h"                                        // KjNode
#include "kjson/kjBuilder.h"                                     // kjChildRemove
}

#include "logMsg/logMsg.h"                                       // LM_*

#include "orionld/common/orionldState.h"                         // orionldState
#include "orionld/common/orionldError.h"                         // orionldError
#include "orionld/types/TreeNode.h"                              // TreeNode
#include "orionld/context/orionldContextItemExpand.h"            // orionldContextItemExpand
#include "orionld/context/orionldAttributeExpand.h"              // orionldAttributeExpand
#include "orionld/payloadCheck/pcheckQ.h"                        // pcheckQ
#include "orionld/payloadCheck/pcheckGeoQ.h"                     // pcheckGeoQ
#include "orionld/payloadCheck/pCheckUri.h"                      // pCheckUri
#include "orionld/payloadCheck/pCheckQuery.h"                    // Own interface



// -----------------------------------------------------------------------------
//
// treeNodeLookup -
//
static TreeNode* treeNodeLookup(TreeNode* treeNodeV, int treeNodes, const char* name)
{
  for (int ix = 0; ix < treeNodes; ix++)
  {
    if (strcmp(treeNodeV[ix].name, name) == 0)
      return &treeNodeV[ix];
  }

  return NULL;
}



// ----------------------------------------------------------------------------
//
// pCheckTreeNodesExtract -
//
static bool pCheckTreeNodesExtract(KjNode* treeP, TreeNode* treeNodeV, int treeNodes, bool errorOnUnknown, const char* detailPrefix)
{
  for (KjNode* nodeP = treeP->value.firstChildP; nodeP != NULL; nodeP = nodeP->next)
  {
    TreeNode* treeNodeP       = treeNodeLookup(treeNodeV, treeNodes, nodeP->name);
    char*     errorTitle      = NULL;  // If set, there's an error
    bool      errorAlreadySet = false;


    //
    // Validity check
    //   - Unknown field  (AND errorOnUnknown == true)
    //   - Duplicated field
    //   - Invalid JSON type  (it's a bitmask)
    //   - Empty Array or Object
    //   - Empty String
    //
    int                      statusCode = 400;
    OrionldResponseErrorType errorType  = OrionldBadRequestData;

    if (treeNodeP == NULL)
    {
      if (errorOnUnknown == true)
        errorTitle = (char*) "Unknown field";
    }
    else if (treeNodeP->aux & NOT_IMPLEMENTED)
    {
      errorTitle = (char*) "This part of the Query is not implpemented";
      statusCode = 501;
      errorType  = OrionldOperationNotSupported;
    }
    else if (treeNodeP->aux & NOT_SUPPORTED)
      errorTitle = (char*) "Field not supported for this type of Query";
    else if (treeNodeP->nodeP != NULL)
      errorTitle = (char*) "Duplicated field";
    else if (((1 << nodeP->type) & treeNodeP->nodeType) == 0)
      errorTitle = (char*) "Invalid JSON type";
    else if (((nodeP->type == KjArray) || (nodeP->type == KjObject)) && (nodeP->value.firstChildP == NULL))
      errorTitle = (nodeP->type == KjArray)? (char*) "Empty JSON Array" : (char*) "Empty JSON Object";
    else if ((nodeP->type == KjString) && (nodeP->value.s[0] == 0))
      errorTitle = (char*) "Empty JSON String";
    else if ((treeNodeP->aux & IS_URI) && (pCheckUri(nodeP->value.s, treeNodeP->name, true) == false))
      errorAlreadySet = true;

    if ((errorTitle != NULL) || (errorAlreadySet == true))
    {
      const char* detail = ((treeNodeP != NULL) && treeNodeP->longName != NULL)? treeNodeP->longName : nodeP->name;

      if (errorAlreadySet == false)
        orionldError(errorType, errorTitle, detail, statusCode);
      return false;
    }

    treeNodeP->nodeP = nodeP;
  }

  //
  // Check for "MANDATORY but missing"
  //
  int ix = 0;
  while ((ix < treeNodes) && (treeNodeV[ix].name != NULL))
  {
    if ((treeNodeV[ix].aux == MANDATORY) && (treeNodeV[ix].nodeP == NULL))
    {
      const char* detail = (treeNodeV[ix].longName != NULL)? treeNodeV[ix].longName : treeNodeV[ix].name;
      orionldError(OrionldBadRequestData, "Mandatory field missing", detail, 400);
      return false;
    }

    ++ix;
  }

  return true;
}



// -----------------------------------------------------------------------------
//
// pCheckEntities -
//
static bool pCheckEntities(KjNode* entitiesP)
{
  // Array check already done (pCheckQuery)

  for (KjNode* entityP = entitiesP->value.firstChildP; entityP != NULL; entityP = entityP->next)
  {
    TreeNode treeNodeV[3] =
    {
      { "id",        "entities:id",        NULL, 1 << KjString,  IS_URI    },
      { "idPattern", "entities:idPattern", NULL, 1 << KjString,  0         },
      { "type",      "entities:type",      NULL, 1 << KjString,  MANDATORY }
    };

    entityP->name = (char*) "entities item";

    if (pCheckTreeNodesExtract(entityP, treeNodeV, 3, true, NULL) == false)
      return false;

    // pCheckTreeNodesExtract guarantees we have a String in treeNodeV[2] - it's the entity type and we need to EXPAND it
    treeNodeV[2].nodeP->value.s = orionldContextItemExpand(orionldState.contextP, treeNodeV[2].nodeP->value.s, true, NULL);

    // According to the spec, id takes precedence over idPattern, so, if both are present, idPattern is NULLed out
    if ((treeNodeV[1].nodeP != NULL) && (treeNodeV[0].nodeP != NULL))
      kjChildRemove(entityP, treeNodeV[1].nodeP);
  }

  return true;
}



// -----------------------------------------------------------------------------
//
// pCheckAttrs -
//
// FIXME:  There are plenty of String Arrays ...
//         Perhaps I should incorporate this functionality inside pCheckTreeNodesExtract ?
//         - if (nodeP->aux && STRING_ARRAY) ...
//
static bool pCheckAttrs(KjNode* attrsArray)
{
  for (KjNode* attrP = attrsArray->value.firstChildP; attrP != NULL; attrP = attrP->next)
  {
    if (attrP->type != KjString)
    {
      orionldError(OrionldBadRequestData, "Invalid JSON Type (must be a JSON String)", "attrs array item", 400);
      return false;
    }

    attrP->value.s = orionldAttributeExpand(orionldState.contextP, attrP->value.s, true, NULL);
  }

  return true;
}



// ----------------------------------------------------------------------------
//
// treeNodeSet -
//
void treeNodeSet(TreeNode* treeNodeP, const char* name, const char* longName, int nodeTypeBitmask, int aux)
{
  treeNodeP->name     = name;
  treeNodeP->longName = longName;
  treeNodeP->nodeP    = NULL;      // Output - points to the KjNode
  treeNodeP->nodeType = nodeTypeBitmask;
  treeNodeP->aux      = aux;
  treeNodeP->output   = NULL;
}



// ----------------------------------------------------------------------------
//
// pCheckQuery -
//
TreeNode* pCheckQuery(KjNode* queryP)
{
  if (queryP->type != KjObject)
  {
    orionldError(OrionldBadRequestData, "Not a JSON Object", "POST Query Payload body", 400);
    return NULL;
  }

  TreeNode* treeNodeV = (TreeNode*) kaAlloc(&orionldState.kalloc, 10 * sizeof(TreeNode));
  if (treeNodeV == NULL)
  {
    orionldError(OrionldInternalError, "Out of memory", "allocating TreeNode array", 500);
    return NULL;
  }

  treeNodeSet(&treeNodeV[0], "type",      NULL, 1 << KjString,  MANDATORY);
  treeNodeSet(&treeNodeV[1], "entities",  NULL, 1 << KjArray,   0);
  treeNodeSet(&treeNodeV[2], "attrs",     NULL, 1 << KjArray,   0);
  treeNodeSet(&treeNodeV[3], "q",         NULL, 1 << KjString,  0);
  treeNodeSet(&treeNodeV[4], "geoQ",      NULL, 1 << KjObject,  0);
  treeNodeSet(&treeNodeV[5], "local",     NULL, 1 << KjBoolean, 0);
  treeNodeSet(&treeNodeV[6], "csf",       NULL, 1 << KjString,  NOT_SUPPORTED);
  treeNodeSet(&treeNodeV[7], "temporalQ", NULL, 1 << KjObject,  NOT_SUPPORTED);
  treeNodeSet(&treeNodeV[8], "scopeQ",    NULL, 1 << KjString,  NOT_IMPLEMENTED);
  treeNodeSet(&treeNodeV[9], "lang",      NULL, 1 << KjString,  0);

  //
  // Extract first level nodes + check for unknown fields and duplicates
  //
  if (pCheckTreeNodesExtract(queryP, treeNodeV, 10, true, NULL) == false)
    return NULL;

  // Make sure "type": "Query" - we already know it's there and is a String
  if (strcmp(treeNodeV[0].nodeP->value.s, "Query") != 0)
  {
    orionldError(OrionldBadRequestData, "Invalid request", "The type field must have the value 'Query'", 400);
    return NULL;
  }


  //
  // Go over first level complex nodes and call their respective pCheck function
  // The simpler fields, e.g. local or lang have already been completely checked by pCheckTreeNodesExtract
  //
  if ((treeNodeV[1].nodeP != NULL) && (pCheckEntities(treeNodeV[1].nodeP)          == false))                         return NULL;
  if ((treeNodeV[2].nodeP != NULL) && (pCheckAttrs(treeNodeV[2].nodeP)             == false))                         return NULL;
  if ((treeNodeV[3].nodeP != NULL) && ((treeNodeV[3].output = pcheckQ(treeNodeV[3].nodeP->value.s))  == NULL)) return NULL;
  if ((treeNodeV[4].nodeP != NULL) && ((treeNodeV[4].output = pcheckGeoQ(&orionldState.kalloc, treeNodeV[4].nodeP, false)) == NULL)) return NULL;

  //
  // Now, for the query to not be too wide, we need at least one of:
  // - entities: ix==1
  // - attrs:    ix==2
  // - q:        ix==3
  // - geoQ:     ix==4
  // - local:    ix==5
  //
  bool tooWide = true;
  for (int ix = 1; ix <= 5; ix++)
  {
    if (treeNodeV[ix].nodeP != NULL)
    {
      tooWide = false;
      break;
    }
  }

  if (tooWide == true)
  {
    orionldError(OrionldBadRequestData, "Invalid request", "the query is too broad (need one of entities, attrs, q, geoQ, local)", 400);
    return NULL;
  }

  return treeNodeV;
}
