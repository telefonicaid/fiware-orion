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
#include "kbase/kMacros.h"                                       // K_VEC_SIZE, K_FT
#include "kbase/kStringSplit.h"                                  // kStringSplit
#include "kbase/kStringArrayJoin.h"                              // kStringArrayJoin
#include "kbase/kStringArrayLookup.h"                            // kStringArrayLookup
#include "kalloc/kaStrdup.h"                                     // kaStrdup
#include "kjson/KjNode.h"                                        // KjNode
#include "kjson/kjBuilder.h"                                     // kjObject, ...
#include "kjson/kjParse.h"                                       // kjParse
}

#include "logMsg/logMsg.h"                                       // LM_*
#include "logMsg/traceLevels.h"                                  // Lmt*

#include "rest/ConnectionInfo.h"                                 // ConnectionInfo
#include "mongoBackend/mongoQueryContext.h"                      // mongoQueryContext

#include "orionld/common/SCOMPARE.h"                             // SCOMPAREx
#include "orionld/common/urlCheck.h"                             // urlCheck
#include "orionld/common/urnCheck.h"                             // urnCheck
#include "orionld/common/orionldState.h"                         // orionldState
#include "orionld/common/orionldErrorResponse.h"                 // orionldErrorResponseCreate
#include "orionld/common/orionldRequestSend.h"                   // orionldRequestSend
#include "orionld/common/dotForEq.h"                             // dotForEq
#include "orionld/context/orionldContextItemAliasLookup.h"       // orionldContextItemAliasLookup
#include "orionld/db/dbConfiguration.h"                          // dbRegistrationLookup, dbEntityRetrieve
#include "orionld/kjTree/kjTreeFromQueryContextResponse.h"       // kjTreeFromQueryContextResponse
#include "orionld/kjTree/kjTreeRegistrationInfoExtract.h"        // kjTreeRegistrationInfoExtract
#include "orionld/context/orionldContextItemExpand.h"            // orionldContextItemExpand
#include "orionld/serviceRoutines/orionldGetEntity.h"            // Own Interface



// -----------------------------------------------------------------------------
//
// attrsToAlias -
//
static void attrsToAlias(OrionldContext* contextP, char* attrV[], int attrs)
{
  for (int ix = 0; ix < attrs; ix++)
  {
    attrV[ix] = orionldContextItemAliasLookup(contextP, attrV[ix], NULL, NULL);
  }
}



// -----------------------------------------------------------------------------
//
// orionldForwardGetEntity2 -
//
// An NGSI-LD Registration looks like this:
//
// {
//   "id": "urn:ngsi-ld:ContextSourceRegistration:csr01",
//   "type": "ContextSourceRegistration",
//   "information": [
//     {
//       "entities": [
//         {
//           "id": "urn:ngsi-ld:entities:E1",
//           "type": "Entity"
//         }
//       ],
//       "properties": [ "P1", "P2", ... ],
//       "relationships": [ "R1", ... ]
//     }
//   ],
//   "endpoint": "http://localhost:9801"
// }
//
// I.e., each registration have:
// - ONE Endpoint
// - An array of entity/attribute combinations
//
// -----------------------------------------------------------------------
//
// In the NGSIv1 database model, the registration is stored like this:
//
//  {
//    ...
//    "contextRegistration" : [
//      {
//        "entities" : [
//          {
//            "id" : "urn:ngsi-ld:entities:E1",
//            "type" : "https://uri.etsi.org/ngsi-ld/default-context/Entity"
//          }
//        ],
//        "attrs" : [
//          {
//            "name" : "https://uri.etsi.org/ngsi-ld/default-context/P1",
//            "type" : "Property",
//          },
//          {
//            "name" : "https://uri.etsi.org/ngsi-ld/default-context/P2",
//            "type" : "Property",
//          },
//          {
//            "name" : "https://uri.etsi.org/ngsi-ld/default-context/R1",
//            "type" : "Relationship",
//          }
//        ],
//        "providingApplication" : "http://my.csource.org:1026"
//      }
//    ], ...
//
// In the database, every contextRegistration item can have its own "providingApplication", however, the incoming payload of NGSi-LD Registrations
// doesn't allow this, so ALL the contextRegistration items will forcibly have the same "providingApplication" ("endpoint" in NGSI-LD).
//
// We assume that one entity is registered for one endpoint only once.
// => There will be ONE forwarded request per matching registration.
//
// As all forwarded requests will return "a piece" of the entity, the Entity::ID and Entity::Type will be stripped off the response, so that only the attributes
// remain and then those attributes will be merged into the "Response Entity".
//
// Let's skip the Entity Type completely. If the Entity ID matches, we're good.
//
// So, we'll need to merge the attribute names into a comma-separated list, for the URI parameter "attrs"
// And especially, if the original GET operation contained the attrs URI param, we need to not include those.
// Four cases:
//
//   URI param 'attrs' present in Original Request     Attribute List in the Registration     Action
//   --------------------------------------------      ----------------------------------     --------------------------------------
//   NO                                                NO                                     No "attrs" URI param in the forwarded request
//   NO                                                YES                                    "attrs" URI param exact copy of "Attribute List in the Registration"
//   YES                                               NO                                     "attrs" URI param exact copy of "URI param 'attrs' in Original Request"
//   YES                                               YES                                    "attrs" URI param is a merge between the two
//
//
static KjNode* orionldForwardGetEntityPart(KjNode* registrationP, char* entityId, char** uriParamAttrV, int uriParamAttrs)
{
  char            host[128]                  = { 0 };
  char            protocol[32]               = { 0 };
  unsigned short  port                       = 0;
  char*           uriDir                     = (char*) "";
  char*           detail;
  KjNode*         entityP                    = NULL;
  char*           registrationAttrV[100];
  int             registrationAttrs          = 0;

  if (kjTreeRegistrationInfoExtract(registrationP, protocol, sizeof(protocol), host, sizeof(host), &port, &uriDir, registrationAttrV, 100, &registrationAttrs, &detail) == false)
    return NULL;

  char* newUriParamAttrsString = (char*) kaAlloc(&orionldState.kalloc, 200 * 30);  // Assuming max 20 attrs, max 200 chars per attr ...

  if ((uriParamAttrs == 0) && (registrationAttrs == 0))
    newUriParamAttrsString = (char*) "";
  else if ((uriParamAttrs == 0) && (registrationAttrs != 0))
  {
    newUriParamAttrsString = (char*) kaAlloc(&orionldState.kalloc, registrationAttrs * 200);
    attrsToAlias(orionldState.contextP, registrationAttrV, registrationAttrs);
    kStringArrayJoin(newUriParamAttrsString, registrationAttrV, registrationAttrs, ",");
  }
  else if ((uriParamAttrs != 0) && (registrationAttrs == 0))
  {
    newUriParamAttrsString = (char*) kaAlloc(&orionldState.kalloc, uriParamAttrs * 200);
    attrsToAlias(orionldState.contextP, uriParamAttrV, uriParamAttrs);
    kStringArrayJoin(newUriParamAttrsString, uriParamAttrV, uriParamAttrs, ",");
  }
  else
  {
    char* attrsV[100];
    int   attrs = 0;

    for (int ix = 0; ix < uriParamAttrs; ix++)
    {
      if (kStringArrayLookup(registrationAttrV, registrationAttrs, uriParamAttrV[ix]) != -1)
        attrsV[attrs++] = uriParamAttrV[ix];
    }

    attrsToAlias(orionldState.contextP, attrsV, attrs);
    kStringArrayJoin(newUriParamAttrsString, attrsV, attrs, ",");
  }


  int   size    = 256 + strlen(newUriParamAttrsString);
  char* urlPath = (char*) kaAlloc(&orionldState.kalloc, size);
  char* uriDirP = (char*) ((uriDir == NULL)? "" : uriDir);


  //
  // If the uri directory (from the registration) ends in a slash, then have it removed.
  // It is added in the snpintf further down
  //
  if (uriDirP != NULL)
  {
    int slen = strlen(uriDirP);
    if (uriDirP[slen - 1] == '/')
      uriDirP[slen - 1] = 0;
  }

  if (*newUriParamAttrsString != 0)
  {
    if (orionldState.uriParamOptions.keyValues)
      snprintf(urlPath, size, "%s/ngsi-ld/v1/entities/%s?options=keyValues&attrs=%s", uriDirP, entityId, newUriParamAttrsString);
    else
      snprintf(urlPath, size, "%s/ngsi-ld/v1/entities/%s?attrs=%s", uriDirP, entityId, newUriParamAttrsString);
  }
  else
  {
    if (orionldState.uriParamOptions.keyValues)
      snprintf(urlPath, size, "%s/ngsi-ld/v1/entities/%s?options=keyValues", uriDirP, entityId);
    else
      snprintf(urlPath, size, "%s/ngsi-ld/v1/entities/%s", uriDirP, entityId);
  }

  //
  // Sending the Forwarded request
  //
  orionldState.httpResponse.buf       = NULL;  // orionldRequestSend allocates
  orionldState.httpResponse.size      = 0;
  orionldState.httpResponse.used      = 0;
  orionldState.httpResponse.allocated = false;

  bool tryAgain = false;
  bool reqOk;
  bool downloadFailed;

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
    reqOk = orionldRequestSend(&orionldState.httpResponse, protocol, host, port, "GET", urlPath, 5000, link, &detail, &tryAgain, &downloadFailed, "Accept: application/json", NULL, NULL, 0, headerV);
  }
  else
    reqOk = orionldRequestSend(&orionldState.httpResponse, protocol, host, port, "GET", urlPath, 5000, NULL, &detail, &tryAgain, &downloadFailed, "Accept: application/json", NULL, NULL, 0, headerV);

  if (reqOk)
    entityP = kjParse(orionldState.kjsonP, orionldState.httpResponse.buf);

  return entityP;
}



// -----------------------------------------------------------------------------
//
// orionldForwardGetEntity -
//
static KjNode* orionldForwardGetEntity(ConnectionInfo* ciP, char* entityId, KjNode* regArrayP, KjNode* responseP, bool needEntityType)
{
  //
  // If URI param 'attrs' was used, split the attr-names into an array and expanmd according to @context
  //
  char* uriParamAttrsV[100];
  int   uriParamAttrs = 0;

  if (!ciP->uriParam["attrs"].empty())
  {
    char* uriParamAttrsString = (char*) ciP->uriParam["attrs"].c_str();

    uriParamAttrs = kStringSplit(uriParamAttrsString, ',', uriParamAttrsV, K_VEC_SIZE(uriParamAttrsV));

    //
    // Populate the array uriParamAttrsV with the expanded attribute names
    //
    for (int ix = 0; ix < uriParamAttrs; ix++)
    {
      uriParamAttrsV[ix] = orionldContextItemExpand(orionldState.contextP, uriParamAttrsV[ix], NULL, true, NULL);
    }
  }

  //
  // Treating all hits from the registrations
  //
  for (KjNode* regP = regArrayP->value.firstChildP; regP != NULL; regP = regP->next)
  {
    KjNode*  partTree = orionldForwardGetEntityPart(regP, entityId, uriParamAttrsV, uriParamAttrs);

    if (partTree != NULL)  // Move all attributes from 'partTree' into responseP
    {
      KjNode* nodeP = partTree->value.firstChildP;
      KjNode* next;

      while (nodeP != NULL)
      {
        next = nodeP->next;

        if (SCOMPARE3(nodeP->name, 'i', 'd', 0))
        {
        }
        else if (SCOMPARE5(nodeP->name, 't', 'y', 'p', 'e', 0))
        {
          if (needEntityType)
          {
            //
            // We're taking the entity::type from the Response to the forwarded request
            // because no local entity was found.
            // It could also be taken from the registration.
            //
            kjChildAdd(responseP, nodeP);
            needEntityType = false;
          }
        }
        else
          kjChildAdd(responseP, nodeP);

        nodeP = next;
      }
    }
  }

  return responseP;
}



static char** attrsListToArray(char* attrList, char* attrV[], int attrVecLen)
{
  int items = kStringSplit(attrList, ',', attrV, attrVecLen);

  attrV[items] = NULL;

  for (int ix = 0; ix < items; ix++)
  {
    attrV[ix] = orionldContextItemExpand(orionldState.contextP, attrV[ix], NULL, true, NULL);
    attrV[ix] = kaStrdup(&orionldState.kalloc, attrV[ix]);
    dotForEq(attrV[ix]);
  }

  return attrV;
}



// #define USE_MONGO_BACKEND 0
// ----------------------------------------------------------------------------
//
// orionldGetEntity -
//
// URI params:
// - attrs               (orionldState.uriParams.attrs)
// - options=keyValues   ()
//
// For the NGSI-LD Forwarding scheme to work, we need to check for registrations and not only in the
// local database. For this to work correctly:
// 1. Lookup matching registrations
// 2. Lookup the entity in the local database
// 3. If not found locally and no matching registrations, return a404 Not Found
// 4. If not found locally, we need to extract the entity type from a Context Provider and if that doesn't work, then we can get
//    the entity type from the matching registration.
// 5. Forward the GET /entities/{entityId} to all matching CPs and add the attributes to the entity
// 6. Return the full entity
//
bool orionldGetEntity(ConnectionInfo* ciP)
{
  char*    detail;
  KjNode*  regArray;
  // bool     keyValues = orionldState.uriParamOptions.keyValues;

  //
  // Make sure the ID (orionldState.wildcard[0]) is a valid URI
  //
  if ((urlCheck(orionldState.wildcard[0], &detail) == false) && (urnCheck(orionldState.wildcard[0], &detail) == false))
  {
    LM_W(("Bad Input (Invalid Entity ID - Not a URL nor a URN)"));
    orionldErrorResponseCreate(OrionldBadRequestData, "Invalid Entity ID", "Not a URL nor a URN");
    return false;
  }

  regArray = dbRegistrationLookup(orionldState.wildcard[0], NULL, NULL);

  LM_T(LmtServiceRoutine, ("In orionldGetEntity: %s", orionldState.wildcard[0]));

#ifdef USE_MONGO_BACKEND
  bool                  keyValues = orionldState.uriParamOptions.keyValues;
  EntityId              entityId(orionldState.wildcard[0], "", "false", false);
  QueryContextRequest   request;
  QueryContextResponse  response;

  request.entityIdVector.push_back(&entityId);

  orionldState.httpStatusCode = mongoQueryContext(&request,
                                                  &response,
                                                  orionldState.tenant,
                                                  ciP->servicePathV,
                                                  ciP->uriParam,
                                                  ciP->uriParamOptions,
                                                  NULL,
                                                  ciP->apiVersion);

  if (response.errorCode.code == SccBadRequest)
  {
    //
    // Not found in local, or some error
    // Get Entity::Type and Entity::ID from the registration (if found)
    // Add the forwardTree to the entity and return it
    // If no registration found, retuirn 404 Not Found
    //
    LM_E(("ToDo: Implement this special case of entity not found in local BUT in registration"));
    orionldErrorResponseCreate(OrionldBadRequestData, "Bad Request", NULL);
    return false;
  }

  // It's OK to not find the Entity in local - we still may find it in a Context Provider
  if ((response.errorCode.code == SccOk) || (response.errorCode.code == 0))
  {
    // Create response by converting "QueryContextResponse response" into a KJson tree
    orionldState.responseTree = kjTreeFromQueryContextResponse(true, orionldState.uriParams.attrs, keyValues, &response);
  }
#else
  char*  attrs[100];
  char** attrsP         = NULL;
  bool   attrsMandatory = false;

  if (orionldState.uriParams.attrs != NULL)
  {
    attrsP = (char**) attrsListToArray(orionldState.uriParams.attrs, attrs, 100);
    if (regArray == NULL)  // No matching registrations
      attrsMandatory = true;
  }

  orionldState.responseTree = dbEntityRetrieve(orionldState.wildcard[0], attrsP, attrsMandatory, orionldState.uriParamOptions.sysAttrs, orionldState.uriParamOptions.keyValues, orionldState.uriParams.datasetId);
#endif

  if ((orionldState.responseTree == NULL) && (regArray == NULL))
  {
    if (attrsMandatory == true)
      orionldErrorResponseCreate(OrionldResourceNotFound, "Combination Entity/Attributes Not Found", orionldState.wildcard[0]);
    else
      orionldErrorResponseCreate(OrionldResourceNotFound, "Entity Not Found", orionldState.wildcard[0]);
    orionldState.httpStatusCode = SccContextElementNotFound;  // 404
    return false;
  }


  //
  // GET /ngsi-ld/v1/entities/{entityId} returns a single Entity, not an Array.
  // It is supposed that there is only ONE entity with a certain Enmtity ID in the entire system ...
  //
  // However, a Context Provider (or more) may have a copy of an entity and that same entity may exist in local in the broker ...
  // In the future we need to rethink this, and perhaps return an Array of entities, not just one single entity.
  // We might also want to include the origin (the Context Provider) of the entity as part of the response ...
  //
  // For now, if the entity is found in a Context Provider, the first one that is found will be returned.
  // If nothing found in the Context Providers, then the local database is searched.
  // This way, only one Entity is returned and we don't break the API. For now ...
  //
  if (regArray != NULL)
  {
    bool needEntityType = false;

    if (orionldState.responseTree == NULL)
    {
      KjNode* idNodeP = kjString(orionldState.kjsonP, "id", orionldState.wildcard[0]);

      orionldState.responseTree = kjObject(orionldState.kjsonP, NULL);
      kjChildAdd(orionldState.responseTree, idNodeP);

      needEntityType = true;  // Get it from Forward-response
    }

    orionldForwardGetEntity(ciP, orionldState.wildcard[0], regArray, orionldState.responseTree, needEntityType);
  }

  return true;
}
