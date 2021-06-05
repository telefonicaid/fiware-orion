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
extern "C"
{
#include "kalloc/kaStrdup.h"                                     // kaStrdup
#include "kjson/KjNode.h"                                        // KjNode
#include "kjson/kjLookup.h"                                      // kjLookup
#include "kjson/kjBuilder.h"                                     // kjChildAdd, kjChildRemove
#include "kjson/kjClone.h"                                       // kjClone
}

#include "logMsg/logMsg.h"                                       // LM_*
#include "logMsg/traceLevels.h"                                  // Lmt*

#include "rest/ConnectionInfo.h"                                 // ConnectionInfo
#include "ngsi/ContextElement.h"                                 // ContextElement
#include "mongoBackend/mongoUpdateContext.h"                     // mongoUpdateContext

#include "orionld/common/orionldErrorResponse.h"                 // orionldErrorResponseCreate
#include "orionld/common/orionldState.h"                         // orionldState
#include "orionld/common/SCOMPARE.h"                             // SCOMPAREx
#include "orionld/common/CHECK.h"                                // *CHECK*
#include "orionld/common/orionldRequestSend.h"                   // orionldRequestSend
#include "orionld/common/dotForEq.h"                             // dotForEq
#include "orionld/common/eqForDot.h"                             // eqForDot
#include "orionld/types/OrionldProblemDetails.h"                 // OrionldProblemDetails
#include "orionld/payloadCheck/pcheckUri.h"                      // pcheckUri
#include "orionld/payloadCheck/pcheckAttribute.h"                // pcheckAttribute
#include "orionld/context/orionldCoreContext.h"                  // orionldCoreContextP
#include "orionld/context/orionldContextFromTree.h"              // orionldContextFromTree
#include "orionld/context/orionldContextItemAliasLookup.h"       // orionldContextItemAliasLookup
#include "orionld/context/orionldAttributeExpand.h"              // orionldAttributeExpand
#include "orionld/context/orionldSubAttributeExpand.h"           // orionldSubAttributeExpand
#include "orionld/contextCache/orionldContextCacheLookup.h"      // orionldContextCacheLookup
#include "orionld/kjTree/kjTreeRegistrationInfoExtract.h"        // kjTreeRegistrationInfoExtract
#include "orionld/kjTree/kjTreeToCompoundValue.h"                // kjTreeToCompoundValue
#include "orionld/mongoBackend/mongoAttributeExists.h"           // mongoAttributeExists
#include "orionld/mongoBackend/mongoEntityExists.h"              // mongoEntityExists
#include "orionld/db/dbConfiguration.h"                          // dbRegistrationLookup
#include "orionld/serviceRoutines/orionldPatchAttribute.h"       // Own Interface



// -----------------------------------------------------------------------------
//
// DB_ERROR -
//
#define DB_ERROR(title, detail)                                       \
do {                                                                  \
  LM_E(("Database Error: %s: %s", title, detail));                    \
  orionldState.httpStatusCode = 500;                                  \
  orionldErrorResponseCreate(OrionldInternalError, title, detail);    \
  return false;                                                       \
} while (0)



// -----------------------------------------------------------------------------
//
// orionldPatchAttributeWithDatasetId -
//
// 0. entityId, attrName and datasetId have been checked already
// 1. Check the validity of the incoming payload (except attr type - need DB lookup for that check)
// 2. GET the "dataset" 'datasetIdP->value.s' for attrNameExpandedEq from the DB
// 3. If "attr type" is present in 'inAttribute', make sure it is identical to what's in the "dataset"
// 4. Clone inAttribute for TRoE (are sub-attrs already expanded?)
// 5. Merge - use kjAttributeMerge (will need some altering for datasetId)
// 6. Replace db.entities.entityId.attrs.attrNameExpandedEq with the merged attribute
//
//    orionldMhdConnectionTreat takes care of TRoE
//
bool orionldPatchAttributeWithDatasetId(KjNode* inAttribute, char* entityId, char* attrName, char* attrNameExpandedEq, const char* datasetId)
{
  char* detail;
  if (pcheckAttribute(inAttribute, NULL, false, &detail) == false)
  {
    LM_W(("Bad Input (invalid attribute - %s)", detail));
    return false;
  }

  KjNode* datasetNodeP = dbDatasetGet(entityId, attrNameExpandedEq, datasetId);
  if (datasetNodeP == NULL)
  {
    LM_W(("Bad Input (dataset '%s' not found for attr '%s' in entity '%s')", datasetId, attrNameExpandedEq, entityId));
    orionldState.httpStatusCode = 404;
    orionldErrorResponseCreate(OrionldResourceNotFound, "attribute dataset not found", datasetId);
    return false;
  }

  KjNode* dbInstanceP = datasetNodeP;
  if (datasetNodeP->type == KjArray)
  {
    for (KjNode* nodeP = datasetNodeP->value.firstChildP; nodeP != NULL; nodeP = nodeP->next)
    {
      KjNode* datasetIdP = kjLookup(nodeP, "datasetId");

      if ((datasetId != NULL) && (strcmp(datasetIdP->value.s, datasetId) == 0))
        dbInstanceP = nodeP;
    }
  }

  // Remove 'datasetId' from the PATCH object (inAttribute)
  KjNode* nodeP = kjLookup(inAttribute, "datasetId");
  if (nodeP != NULL)
    kjChildRemove(inAttribute, nodeP);

  // Modify 'modifiedAt' for dbInstanceP
  nodeP = kjLookup(dbInstanceP, "modifiedAt");

  if (nodeP != NULL)
    nodeP->value.f = orionldState.requestTime;
  else
  {
    // This should not happen
    nodeP = kjFloat(orionldState.kjsonP, "modifiedAt", orionldState.requestTime);
    kjChildAdd(dbInstanceP, nodeP);
  }

  // For every item in inAttribute, look it up in dbInstanceP and replace if found, else, add (to dbInstanceP)
  //
  // Actually, it's faster to go over all of the nodes in inAttribute and remove those that are found in dbInstanceP.
  // After dbInstanceP is "clean" - just add all the inAttribute children to dbInstanceP
  //
  for (nodeP = inAttribute->value.firstChildP; nodeP != NULL; nodeP = nodeP->next)
  {
    KjNode* toRemove = kjLookup(dbInstanceP, nodeP->name);

    if (toRemove != NULL)
      kjChildRemove(dbInstanceP, toRemove);
  }

  // Now, simply concatenate the two
  dbInstanceP->lastChild->next = inAttribute->value.firstChildP;
  dbInstanceP->lastChild       = inAttribute->lastChild;

  //
  // Create the path to the field we want to update
  //
  char attrDatasetFieldPath[512];
  snprintf(attrDatasetFieldPath, sizeof(attrDatasetFieldPath), "@datasets.%s", attrNameExpandedEq);

  // And finally, ask the mongo driver to do the update
  dbEntityFieldReplace(entityId, attrDatasetFieldPath, datasetNodeP);

  orionldState.httpStatusCode = 204;

  return true;
}



// -----------------------------------------------------------------------------
//
// kjAttributeMerge -
//
bool kjAttributeMerge(KjNode* inAttribute, KjNode* dbAttribute, KjNode* dbAttributeType, KjNode* inAttributeType, KjNode* dbCreatedAt)
{
  //
  // If the value isn't present in inAttribute, get it from the database
  //
  KjNode* valueP = kjLookup(inAttribute, "value");

  if (valueP == NULL)
  {
    KjNode* dbValueP = kjLookup(dbAttribute, "value");
    if (dbValueP == NULL)
    {
      LM_E(("Database Error (no 'value' member in DB for attribute '%s')", dbAttribute->name));
      return false;
    }

    kjChildRemove(dbAttribute, dbValueP);
    kjChildAdd(inAttribute, dbValueP);
  }

  //
  // If the type isn't present in inAttribute, get it from the database
  //
  if (inAttributeType == NULL)
  {
    kjChildRemove(dbAttribute, dbAttributeType);
    kjChildAdd(inAttribute, dbAttributeType);
  }

  //
  // Add the createdAt from the DB
  //
  kjChildAdd(inAttribute, dbCreatedAt);
  dbCreatedAt->name = (char*) "createdAt";

  //
  // Add a modifiedAt to inAttribute
  //
  KjNode* modifiedAtP = kjFloat(orionldState.kjsonP, "modifiedAt", orionldState.requestTime);
  kjChildAdd(inAttribute, modifiedAtP);


  //
  // Get the 'md' list - that's where the sub-attributes are
  // If there is no 'md' field, then there are no sub-attributes
  //
  KjNode* dbMdList = kjLookup(dbAttribute, "md");

  if (dbMdList != NULL)
  {
    KjNode* dbMdP = dbMdList->value.firstChildP;
    KjNode* next;

    while (dbMdP != NULL)
    {
      next = dbMdP->next;

      eqForDot(dbMdP->name);

      //
      // If already present - ignore, else - append
      //
      KjNode* inMdP = kjLookup(inAttribute, dbMdP->name);
      if (inMdP == NULL)
      {
        kjChildRemove(dbMdList, dbMdP);
        kjChildAdd(inAttribute, dbMdP);
      }

      dbMdP = next;
    }
  }

  return true;
}



// -----------------------------------------------------------------------------
//
// kjTreeToAttributeValue -
//
static bool kjTreeToAttributeValue(ContextAttribute* caP, KjNode* valueP, int* levelP)
{
  if (valueP->type == KjString)
  {
    caP->valueType   = orion::ValueTypeString;
    caP->stringValue = valueP->value.s;
  }
  else if (valueP->type == KjBoolean)
  {
    caP->valueType   = orion::ValueTypeBoolean;
    caP->boolValue   = valueP->value.b;
  }
  else if (valueP->type == KjInt)
  {
    caP->valueType   = orion::ValueTypeNumber;
    caP->numberValue = valueP->value.i;
  }
  else if (valueP->type == KjFloat)
  {
    caP->valueType   = orion::ValueTypeNumber;
    caP->numberValue = valueP->value.f;
  }
  else if (valueP->type == KjNull)
    caP->valueType = orion::ValueTypeNull;
  else if (valueP->type == KjObject)
  {
    caP->valueType      = orion::ValueTypeObject;
    caP->compoundValueP = kjTreeToCompoundValue(valueP, NULL, 0);
  }
  else if (valueP->type == KjArray)
  {
    caP->valueType      = orion::ValueTypeObject;  // Array???
    caP->compoundValueP = kjTreeToCompoundValue(valueP, NULL, 0);
  }

  return true;
}



// -----------------------------------------------------------------------------
//
// kjTreeToMetadataValue -
//
static bool kjTreeToMetadataValue(Metadata* metadataP, KjNode* valueP)
{
  if (valueP->type == KjString)
  {
    metadataP->valueType   = orion::ValueTypeString;
    metadataP->stringValue = valueP->value.s;
  }
  else if (valueP->type == KjBoolean)
  {
    metadataP->valueType   = orion::ValueTypeBoolean;
    metadataP->boolValue   = valueP->value.b;
  }
  else if (valueP->type == KjInt)
  {
    metadataP->valueType   = orion::ValueTypeNumber;
    metadataP->numberValue = valueP->value.i;
  }
  else if (valueP->type == KjFloat)
  {
    metadataP->valueType   = orion::ValueTypeNumber;
    metadataP->numberValue = valueP->value.f;
  }
  else if (valueP->type == KjNull)
    metadataP->valueType = orion::ValueTypeNull;
  else if (valueP->type == KjObject)
  {
    metadataP->valueType      = orion::ValueTypeObject;
    metadataP->compoundValueP = kjTreeToCompoundValue(valueP, NULL, 0);
  }
  else if (valueP->type == KjArray)
  {
    metadataP->valueType      = orion::ValueTypeObject;
    metadataP->compoundValueP = kjTreeToCompoundValue(valueP, NULL, 0);
  }

  return true;
}



// -----------------------------------------------------------------------------
//
// orionldForwardPatchAttribute
//
static bool orionldForwardPatchAttribute
(
  ConnectionInfo*  ciP,
  KjNode*          registrationP,
  const char*      entityId,
  const char*      attrName,
  KjNode*          payloadData
)
{
  char            host[128]                  = { 0 };
  char            protocol[32]               = { 0 };
  unsigned short  port                       = 0;
  char*           uriDir                     = NULL;
  char*           detail;
  char*           registrationAttrV[100];
  int             registrationAttrs          = 0;

  if (kjTreeRegistrationInfoExtract(registrationP, protocol, sizeof(protocol), host, sizeof(host), &port, &uriDir, registrationAttrV, 100, &registrationAttrs, &detail) == false)
    return false;

  const char*  contentType = (orionldState.ngsildContent == true)? "application/ld+json" : "application/json";
  int          payloadLen  = strlen(orionldState.requestPayload);
  bool         tryAgain;
  bool         downloadFailed;
  bool         reqOk;
  char         uriPath[512];

  if (orionldState.forwardAttrsCompacted == true)
  {
    KjNode*         regContextNodeP;
    OrionldContext* regContextP = NULL;

    regContextNodeP = kjLookup(registrationP, "@context");

    //
    // For now - if a @context present in the registration, use it
    //           else, use the one of the current request
    //
    if (regContextNodeP != NULL)
    {
      OrionldProblemDetails pd;

      if (regContextNodeP->type == KjString)
        regContextP = orionldContextCacheLookup(regContextNodeP->value.s);

      if (regContextP == NULL)
        regContextP = orionldContextFromTree(NULL, OrionldContextFromInline, NULL, regContextNodeP, &pd);
    }

    if (regContextP == NULL)
      regContextP = orionldState.contextP;

    if (regContextP != NULL)
      attrName = orionldContextItemAliasLookup(regContextP, attrName, NULL, NULL);
  }


  //
  // If the uri directory (from the registration) ends in a slash, then have it removed.
  // It is added in the snprintf further down
  //
  if (uriDir == NULL)
    snprintf(uriPath, sizeof(uriPath), "/ngsi-ld/v1/entities/%s/attrs/%s", entityId, attrName);
  else
  {
    int slen = strlen(uriDir);

    if (uriDir[slen - 1] == '/')
      uriDir[slen - 1] = 0;

    snprintf(uriPath, sizeof(uriPath), "%s/ngsi-ld/v1/entities/%s/attrs/%s", uriDir, entityId, attrName);
  }

  //
  // Prepare HTTP headers
  //
  OrionldHttpHeader headerV[5];
  int               header = 0;

  if ((orionldState.tenant != NULL) && (orionldState.tenant[0] != 0))
  {
    headerV[header].type  = HttpHeaderTenant;
    headerV[header].value = orionldState.tenant;
    ++header;
  }
  if ((orionldState.servicePath != NULL) && (orionldState.servicePath[0] != 0))
  {
    headerV[header].type  = HttpHeaderPath;
    headerV[header].value = orionldState.servicePath;
    ++header;
  }
  headerV[header].type = HttpHeaderNone;

  if (orionldState.linkHttpHeaderPresent)
  {
    char link[512];

    snprintf(link, sizeof(link), "<%s>; rel=\"http://www.w3.org/ns/json-ld#context\"; type=\"application/ld+json\"", orionldState.link);
    reqOk = orionldRequestSend(&orionldState.httpResponse, protocol, host, port, "PATCH", uriPath, 5000, link, &detail, &tryAgain, &downloadFailed, NULL, contentType, orionldState.requestPayload, payloadLen, headerV);
  }
  else
    reqOk = orionldRequestSend(&orionldState.httpResponse, protocol, host, port, "PATCH", uriPath, 5000, NULL, &detail, &tryAgain, &downloadFailed, NULL, contentType, orionldState.requestPayload, payloadLen, headerV);

  if (reqOk == false)
  {
    LM_E(("PATCH: orionldRequestSend failed: %s", detail));
    orionldState.httpStatusCode = 500;  // ???
    return false;
  }

  orionldState.httpStatusCode = 204;

  return true;
}



// ----------------------------------------------------------------------------
//
// dbRegistrationsOnlyOneAllowed -
//
void dbRegistrationsOnlyOneAllowed(KjNode* regArray, int matchingRegs, const char* entityId, const char* attrName)
{
  LM_E(("FATAL ERROR: Found more than one (%d) matching registration for an Entity-Attribute pair - this means the database is inconsistent", matchingRegs));
  LM_E(("The Entity-Attribute pair is: '%s' - '%s'", entityId, attrName));

  for (KjNode* regP = regArray->value.firstChildP; regP != NULL; regP = regP->next)
  {
    KjNode* idNodeP = kjLookup(regP, "id");  // Coming from DB - no '@id' needed

    if (idNodeP != NULL)
      LM_E(("Matching Registration: %s", idNodeP->value.s));
  }
  LM_X(1, ("The database needs to be fixed before starting the broker again."));
}



// -----------------------------------------------------------------------------
//
// kjAttributeToNgsiContextAttribute -
//
bool kjAttributeToNgsiContextAttribute(ContextAttribute* caP, KjNode* inAttribute, char** detailP)
{
  int level = 0;

  for (KjNode* mdP = inAttribute->value.firstChildP; mdP != NULL; mdP = mdP->next)
  {
    if (strcmp(mdP->name, "type") == 0)
      caP->type = mdP->value.s;
    else if (strcmp(mdP->name, "value") == 0)
      kjTreeToAttributeValue(caP, mdP, &level);
    else if (strcmp(mdP->name, "object") == 0)
    {
      caP->valueType   = orion::ValueTypeString;
      caP->stringValue = mdP->value.s;
    }
    else if (strcmp(mdP->name, "createdAt") == 0)
      caP->creDate = mdP->value.f;
    else if (strcmp(mdP->name, "modifiedAt") == 0)
      caP->modDate = mdP->value.f;
    else if (strcmp(mdP->name, "observedAt") == 0)
    {
      //
      // observedAt can come either:
      //   - From the DB (as an object containing a member 'value' that is a floating point value) OR
      //   - From the payload body (as an ISO8601 string)
      //
      Metadata* metadataP = new Metadata();

      metadataP->name        = "observedAt";
      metadataP->valueType   = orion::ValueTypeNumber;

      if (mdP->type == KjString)
        metadataP->numberValue = parse8601Time(mdP->value.s);
      else
      {
        KjNode* valueP         = mdP->value.firstChildP;
        metadataP->numberValue = valueP->value.f;
      }

      caP->metadataVector.push_back(metadataP);
    }
    else if (strcmp(mdP->name, "unitCode") == 0)
    {
      //
      // Just like observedAt, unitCode can come either as an object or as a string
      //
      Metadata* metadataP = new Metadata();

      metadataP->name        = "unitCode";
      metadataP->valueType   = orion::ValueTypeString;

      if (mdP->type == KjString)
        metadataP->stringValue = mdP->value.s;
      else
      {
        KjNode* valueP         = mdP->value.firstChildP;
        metadataP->stringValue = valueP->value.s;
      }

      caP->metadataVector.push_back(metadataP);
    }
    else
    {
      Metadata* metadataP = new Metadata();
      metadataP->name     = mdP->name;

      for (KjNode* mdItemP = mdP->value.firstChildP; mdItemP != NULL; mdItemP = mdItemP->next)
      {
        if (strcmp(mdItemP->name, "value") == 0)
          kjTreeToMetadataValue(metadataP, mdItemP);
        else if (strcmp(mdItemP->name, "object") == 0)
        {
          metadataP->valueType   = orion::ValueTypeString;
          metadataP->stringValue = mdItemP->value.s;
        }
        else if (strcmp(mdItemP->name, "type") == 0)
          metadataP->type = mdItemP->value.s;
      }

      caP->metadataVector.push_back(metadataP);
    }
  }

  return true;
}



// ----------------------------------------------------------------------------
//
// orionldPatchAttribute -
//
// 1.  Check wildcards for validity (entity ID + attribute name) + remove ev. builtins from payload body
// 2.  Query registrations and forward message if found
// 3.  Lookup 'datasetId' in inAttribute - if found, enter datasetId function
// 4.  GET the attribute from the DB (dbAttribute)
// 5.  404 if not found in DB
// 6.  Extract type and createdAt from dbAttribute
// 7.  Check that inAttribute is OK (especially the attribute type)
// 8.  Merge dbAttribute into inAttribute
// 9.  Convert merged inAttribute into a ContextElement
// 10. DELETE the attribute from the DB
// 11. Call mongoBackend
//
// ETSI Spec: 5.6.4 Partial Attribute update
//   This operation allows performing a partial update on an NGSI-LD Entity's Attribute (Property or Relationship).
//   A partial update only changes the elements provided in an Entity Fragment, leaving the rest as they are.
//
//   =>
//   1. Check payload correctness
//   2. If matching registration - forward, await response and return that response to caller
//   3. Convert KjNode tree to UpdateContextRequest
//   ?. Perhaps GET the entity+attribute first and merge with incoming payload KjNode tree
//   5. mongoUpdateContext with Update (Append?)
//
// About Forwarding
//   A PATCH can have only ONE match in the registrations.
//   If more than one, then the database is inconsistent and must be fixed.
//
//   If a match is found (one match) in the registraions, then nothing is done locally,
//   the exact same payload will be forwarded as is.
//   => No need to parse
//
//   So, for PATCH Attribute, the parsing of the payload should not be done until after
//   we have seen that there are no matching registration.
//
// FIXME: Invent a new ServiceCharacteristics to NOT parse the payload and use it for orionldPatchAttribute
// FIXME: Encapsulate the parsing of the payload and extraction of eventual @context payload member so it's easy to use from inside orionldPatchAttribute.
//
bool orionldPatchAttribute(ConnectionInfo* ciP)
{
  char*    entityId      = orionldState.wildcard[0];
  char*    attrName      = orionldState.wildcard[1];
  KjNode*  inAttribute   = orionldState.requestTree;
  char*    detail;

  //
  // 1.1 Make sure the ID (orionldState.wildcard[0]) is a valid URI
  //
  if (pcheckUri(entityId, true, &detail) == false)
  {
    LM_W(("Bad Input (Invalid Entity ID '%s' - Not a URI)", entityId));
    orionldErrorResponseCreate(OrionldBadRequestData, "Invalid Entity ID", "Not a URI");  // FIXME: Include 'detail' and name (entityId)
    return false;
  }

  //
  // 1.2 Make sure the attrName (orionldState.wildcard[1]) is a valid NAME or URI
  //
  if (pcheckUri(attrName, false, &detail) == false)
  {
    LM_W(("Bad Input (Invalid Attribute Name '%s' - %s)", attrName, detail));
    orionldErrorResponseCreate(OrionldBadRequestData, "Invalid Attribute Name", detail);
    return false;
  }


  //
  // 1.3 If createdAt or modifiedAt are found in the incoming payload body - remove them
  //
  KjNode* nodeP;

  nodeP = kjLookup(inAttribute, "createdAt");
  if (nodeP != NULL)
    kjChildRemove(inAttribute, nodeP);

  nodeP = kjLookup(inAttribute, "modifiedAt");
  if (nodeP != NULL)
    kjChildRemove(inAttribute, nodeP);


  char* attrNameExpanded   = orionldAttributeExpand(orionldState.contextP, attrName, true, NULL);


  //
  // 2. Query registrations and forward message if found - let forward-receiver check the validity
  //
  if (forwarding)
  {
    //
    // If a matching registration is found, no local treatment will be done.
    // The request is simply forwarded to the matching Context Provider
    //
    int     matchingRegs;
    KjNode* regArray = dbRegistrationLookup(entityId, attrNameExpanded, &matchingRegs);

    if (regArray != NULL)
    {
      //
      // There can be only ONE matching registration - the second matching should not have been allowed
      //
      if (matchingRegs > 1)
        dbRegistrationsOnlyOneAllowed(regArray, matchingRegs, entityId, attrName);

      orionldState.noDbUpdate = true;  // No TRoE for data that is not local in the broker
      return orionldForwardPatchAttribute(ciP, regArray->value.firstChildP, entityId, attrName, inAttribute);
    }
  }


  char* attrNameExpandedEq = kaStrdup(&orionldState.kalloc, attrNameExpanded);
  dotForEq(attrNameExpandedEq);

  //
  // 3. Lookup 'datasetId' in inAttribute - if found, enter datasetId function
  //
  KjNode* datasetIdP = kjLookup(inAttribute, "datasetId");
  if (datasetIdP != NULL)
  {
    STRING_CHECK(datasetIdP, "datasetId");
    URI_CHECK(datasetIdP->value.s, "datasetId", true);

    return orionldPatchAttributeWithDatasetId(inAttribute, entityId, attrName, attrNameExpandedEq, datasetIdP->value.s);
  }

  //
  // 4. GET the attribute from the DB (dbAttribute)
  // 5. 404 if not found in DB
  //
  KjNode* dbEntityP = dbEntityAttributeLookup(entityId, attrNameExpanded);
  if (dbEntityP == NULL)
  {
    char pair[1024];

    snprintf(pair, sizeof(pair), "Entity '%s', Attribute '%s' (%s)", entityId, attrName, attrNameExpandedEq);
    orionldState.httpStatusCode = 404;  // Not Found
    orionldErrorResponseCreate(OrionldResourceNotFound, "Entity/Attribute not found", pair);
    return false;
  }

  // Get "attrs" member of the DB entity
  KjNode* dbAttrs = kjLookup(dbEntityP, "attrs");
  if (dbAttrs == NULL)
    DB_ERROR("Database Error (entity field not found)", "attrs");


  // Get the attribute named attrNameExpandedEq from the "attrs" field of the DB Entity
  KjNode* dbAttribute = kjLookup(dbAttrs, attrNameExpandedEq);
  if (dbAttribute == NULL)
    DB_ERROR("Database Error (attribute not found)", attrNameExpandedEq);


  //
  // 6. Extract type and createdAt from dbAttribute
  //
  KjNode* dbAttributeType = kjLookup(dbAttribute, "type");
  if (dbAttributeType == NULL)
    DB_ERROR("Database Error (attribute member not found)", "type");
  kjChildRemove(dbAttribute, dbAttributeType);

  KjNode* dbCreatedAt = kjLookup(dbAttribute, "creDate");
  if (dbCreatedAt == NULL)
    DB_ERROR("Database Error (attribute member not found)", "creDate");
  kjChildRemove(dbAttribute, dbCreatedAt);

  //
  // 7. Check that inAttribute is OK (especially the attribute type)
  //
  KjNode* inType = kjLookup(inAttribute, "type");
  if (inType != NULL)
  {
    if (inType->type != KjString)
    {
      LM_W(("Bad Input (attribute type must be a string)"));
      orionldState.httpStatusCode = 400;
      orionldErrorResponseCreate(OrionldBadRequestData, "Invalid Attribute Type - must be a string", attrName);
      return false;
    }

    if (strcmp(inType->value.s, dbAttributeType->value.s) != 0)
    {
      LM_W(("Bad Input (input attribute type differs from the one in the database - '%s' vs '%s')", inType->value.s, dbAttributeType->value.s));
      orionldState.httpStatusCode = 400;
      orionldErrorResponseCreate(OrionldBadRequestData, "Invalid Attribute Type", "attribute type cannot be modified");
      return false;
    }
  }

  // Expand all sub-attributes
  for (KjNode* saP = inAttribute->value.firstChildP; saP != NULL; saP = saP->next)
  {
    saP->name = orionldSubAttributeExpand(orionldState.contextP, saP->name, true, NULL);
  }

  if (pcheckAttribute(inAttribute, dbAttributeType->value.s, false, &detail) == false)
  {
    LM_W(("Bad Input (invalid attribute - %s)", detail));
    orionldState.httpStatusCode = 400;
    orionldErrorResponseCreate(OrionldBadRequestData, "Invalid Attribute", detail);
    return false;
  }


  //
  // Save the incoming tree for TRoE
  //
  if (troe)
  {
    KjNode* troeTree = kjClone(orionldState.kjsonP, inAttribute);

    troeTree->name = attrNameExpanded;

    if (inType == NULL)  // No attribute type present in the incoming payload body
    {
      KjNode* typeNodeP = kjString(orionldState.kjsonP, "type", dbAttributeType->value.s);

      kjChildAdd(troeTree, typeNodeP);
    }

    orionldState.requestTree = troeTree;
  }


  //
  // 8. Merge dbAttribute into inAttribute
  //
  kjAttributeMerge(inAttribute, dbAttribute, dbAttributeType, inType, dbCreatedAt);


  //
  // 9.  Convert merged inAttribute into a ContextElement
  //
  ContextAttribute* caP = new ContextAttribute();

  caP->name = attrNameExpanded;
  if (kjAttributeToNgsiContextAttribute(caP, inAttribute, &detail) == false)
  {
    delete caP;

    LM_E(("kjAttributeToNgsiContextAttribute failed: %s", detail));
    orionldErrorResponseCreate(OrionldBadRequestData, "Internal Error", "Unable to convert merged attribute to a struct for mongoBackend");
    return false;
  }

  UpdateContextRequest   ucr;
  ContextElement*        ceP = new ContextElement();

  ceP->entityId.id = entityId;
  // GET entity type from dbEntityP::_id::type ?
  ceP->entityId.servicePath = "/";

  // Add ceP to ucr
  ucr.contextElementVector.push_back(ceP);
  // Add caP to ceP
  ceP->contextAttributeVector.push_back(caP);


  //
  // 10. DELETE the attribute from the DB?
  //

  //
  // 11. Call mongoBackend
  //
  UpdateContextResponse  mongoResponse;

  ucr.updateActionType        = ActionTypeUpdate;
  orionldState.httpStatusCode = mongoUpdateContext(&ucr,
                                                   &mongoResponse,
                                                   orionldState.tenant,
                                                   ciP->servicePathV,
                                                   ciP->uriParam,
                                                   ciP->httpHeaders.xauthToken.c_str(),
                                                   ciP->httpHeaders.correlator.c_str(),
                                                   ciP->httpHeaders.ngsiv2AttrsFormat.c_str(),
                                                   ciP->apiVersion,
                                                   NGSIV2_NO_FLAVOUR);

  if (orionldState.httpStatusCode == 200)
    orionldState.httpStatusCode = 204;
  else
  {
    LM_E(("mongoUpdateContext: HTTP Status Code: %d", orionldState.httpStatusCode));
    orionldErrorResponseCreate(OrionldBadRequestData, "Internal Error", "Error from Mongo-DB backend");
    orionldState.httpStatusCode = 500;
    ucr.release();
    return false;
  }

  ucr.release();

  return true;
}
