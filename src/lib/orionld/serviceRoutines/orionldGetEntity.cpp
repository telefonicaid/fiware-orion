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
#include "kbase/kMacros.h"                                       // K_VEC_SIZE
#include "kbase/kStringSplit.h"                                  // kStringSplit
#include "kbase/kStringArrayJoin.h"                              // kStringArrayJoin
#include "kbase/kStringArrayLookup.h"                            // kStringArrayLookup
#include "kjson/KjNode.h"                                        // KjNode
#include "kjson/kjBuilder.h"                                     // kjObject, ...
#include "kjson/kjLookup.h"                                      // kjLookup
#include "kjson/kjParse.h"                                       // kjParse
}

#include "logMsg/logMsg.h"                                       // LM_*
#include "logMsg/traceLevels.h"                                  // Lmt*

#include "rest/ConnectionInfo.h"                                 // ConnectionInfo
#include "mongoBackend/mongoQueryContext.h"                      // mongoQueryContext

#include "orionld/common/SCOMPARE.h"                             // SCOMPAREx
#include "orionld/common/urlCheck.h"                             // urlCheck
#include "orionld/common/urlParse.h"                             // urlParse
#include "orionld/common/urnCheck.h"                             // urnCheck
#include "orionld/common/orionldState.h"                         // orionldState
#include "orionld/common/orionldErrorResponse.h"                 // orionldErrorResponseCreate
#include "orionld/common/orionldRequestSend.h"                   // orionldRequestSend
#include "orionld/kjTree/kjTreeFromQueryContextResponse.h"       // kjTreeFromQueryContextResponse
#include "orionld/context/orionldContextItemExpand.h"            // orionldContextItemExpand
#include "orionld/serviceRoutines/orionldGetEntity.h"            // Own Interface


#include "mongo/client/dbclient.h"                               // MongoDB C++ Client Legacy Driver
#include "mongoBackend/MongoGlobal.h"                            // getMongoConnection, releaseMongoConnection, ...
#include "orionld/db/dbCollectionPathGet.h"                      // dbCollectionPathGet
#include "orionld/db/dbConfiguration.h"                          // dbDataToKjTree



// -----------------------------------------------------------------------------
//
// mongoCppLegacyRegistrationLookup - FIXME: move to src/lib/orionld/mongoCppLegacy/mongoCppLegacyRegistrationLookup.cpp
//
static KjNode* mongoCppLegacyRegistrationLookup(char* entityId)
{
  //
  // Query registrations collection for:
  //   db.registrations.find({ "contextRegistration.entities.id": "urn:ngsi-ld:entities:E1" })
  //
  // This part is to be moved to "src/lib/orionld/mongoCppLegacy/" once working ...
  //
  char    collectionPath[256];
  KjNode* kjRegArray = NULL;

  dbCollectionPathGet(collectionPath, sizeof(collectionPath), "registrations");

  //
  // Populate filter - only Entity ID for this operation - FOR NOW ...
  //
  mongo::BSONObjBuilder  filter;
  filter.append("contextRegistration.entities.id", entityId);

  // semTake()
  mongo::DBClientBase*                  connectionP = getMongoConnection();
  std::auto_ptr<mongo::DBClientCursor>  cursorP;
  mongo::Query                          query(filter.obj());

  cursorP = connectionP->query(collectionPath, query);

  while (cursorP->more())
  {
    mongo::BSONObj  bsonObj = cursorP->nextSafe();
    char*           title;
    char*           details;
    KjNode*         kjTree = dbDataToKjTree(&bsonObj, &title, &details);

    if (kjTree == NULL)
      LM_E(("%s: %s", title, details));
    else
    {
      if (kjRegArray == NULL)
        kjRegArray = kjArray(orionldState.kjsonP, NULL);
      kjChildAdd(kjRegArray, kjTree);
    }
  }

  releaseMongoConnection(connectionP);

  // semGive()

  return kjRegArray;
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
static KjNode* orionldForwardGetEntity2(KjNode* regP, char* entityId, char** uriParamAttrV, int uriParamAttrs)
{
  LM_TMP(("FWD: uriParamAttrs: %d", uriParamAttrs));
  for (int ix = 0; ix < uriParamAttrs; ix++)
    LM_TMP(("FWD: uriParamAttrV[%d]: '%s'", ix, uriParamAttrV[ix]));

  char*           entityType            = NULL;
  char            host[128]             = { 0 };
  char            protocol[32]          = { 0 };
  unsigned short  port                  = 0;
  char*           uriDir;
  char*           detail;
  KjNode*         entityP               = NULL;
  int             ix                    = 0;
  int             crIx                  = 0;
  char*           registrationAttrV[100];
  int             registrationAttrs     = 0;
  char*           format                = (char*) "JSON";
  KjNode*         contextP              = NULL;

  for (KjNode* nodeP = regP->value.firstChildP; nodeP != NULL; nodeP = nodeP->next)
  {
    LM_TMP(("FWD: registration field %d: %s", ix, nodeP->name));
    ++ix;

    if (strcmp(nodeP->name, "format") == 0)
    {
      format = nodeP->value.s;
    }
    else if (strcmp(nodeP->name, "@context") == 0)
    {
      contextP = nodeP;
    }
    else if (strcmp(nodeP->name, "contextRegistration") == 0)
    {
      for (KjNode* crNodeP = nodeP->value.firstChildP; crNodeP != NULL; crNodeP = crNodeP->next)
      {
        LM_TMP(("FWD: contextRegistration Array Item %d", crIx));
        int crItemIx = 0;

        for (KjNode* crItemNodeP = nodeP->value.firstChildP->value.firstChildP; crItemNodeP != NULL; crItemNodeP = crItemNodeP->next)
        {
          if (strcmp(crItemNodeP->name, "entities") == 0)
          {
            // For now, there can only be one:
            for (KjNode* entityNodeP = crItemNodeP->value.firstChildP->value.firstChildP; entityNodeP != NULL; entityNodeP = entityNodeP->next)
            {
              LM_TMP(("FWD: contextRegistration.entities.%s: %s", entityNodeP->name, entityNodeP->value.s));
              if (strcmp(entityNodeP->name, "type") == 0)
                entityType = entityNodeP->value.s;
            }
          }
          else if (strcmp(crItemNodeP->name, "attrs") == 0)
          {
            //
            // Populate the array registrationAttrV with the registered attributes
            //
            for (KjNode* attrItemP = crItemNodeP->value.firstChildP; attrItemP != NULL; attrItemP = attrItemP->next)
            {
              KjNode* nameP = kjLookup(attrItemP, "name");

              if (nameP == NULL)
              {
                LM_W(("FWD: 'name' field not found in attrs array item"));
                continue;
              }

              registrationAttrV[registrationAttrs] = nameP->value.s;
              ++registrationAttrs;
            }
          }
          else if ((host[0] == 0) && (strcmp(crItemNodeP->name, "providingApplication") == 0))
          {
            LM_TMP(("FWD: providingApplication: %s (must be parsed)", crItemNodeP->value.s));
            if (urlParse(crItemNodeP->value.s, protocol, sizeof(protocol), host, sizeof(host), &port, &uriDir, &detail) == false)
            {
              // Mark Error so that "Incomplete Response" is present in response?
              return NULL;
            }
          }
          else
            LM_TMP(("FWD: contextRegistration[%d].%s: %s", crItemIx, crItemNodeP->name, kjValueType(crItemNodeP->type)));

          ++crItemIx;
        }
      }
    }
  }

  char* newUriParamAttrsString = (char*) kaAlloc(&orionldState.kalloc, 200 * 30);  // Assuming max 20 attrs, max 200 chars per attr ...

  if ((uriParamAttrs == 0) && (registrationAttrs == 0))
    newUriParamAttrsString = (char*) "";
  else if ((uriParamAttrs == 0) && (registrationAttrs != 0))
  {
    LM_TMP(("FWD: Flatten registrationAttrV"));
    newUriParamAttrsString = (char*) kaAlloc(&orionldState.kalloc, registrationAttrs * 200);
    kStringArrayJoin(newUriParamAttrsString, registrationAttrV, registrationAttrs, ",");
  }
  else if ((uriParamAttrs != 0) && (registrationAttrs == 0))
  {
    LM_TMP(("FWD: Flatten uriParamAttrV"));
    newUriParamAttrsString = (char*) kaAlloc(&orionldState.kalloc, uriParamAttrs * 200);
    kStringArrayJoin(newUriParamAttrsString, uriParamAttrV, uriParamAttrs, ",");
  }
  else
  {
    LM_TMP(("FWD: Flatten common attrs in registrationAttrV and uriParamAttrs"));
    char* attrsV[100];
    int   attrs = 0;

    for (int ix = 0; ix < uriParamAttrs; ix++)
    {
      if (kStringArrayLookup(registrationAttrV, registrationAttrs, uriParamAttrV[ix]) != -1)
        attrsV[attrs++] = uriParamAttrV[ix];
    }
    LM_TMP(("FWD: Calling kStringArrayJoin with %d items in vector", attrs));
    kStringArrayJoin(newUriParamAttrsString, attrsV, attrs, ",");
  }

  LM_TMP(("FWD: entity::id:               %s", entityId));
  LM_TMP(("FWD: entity::type:             %s", entityType));
  LM_TMP(("FWD: attrs to forward:         %s", newUriParamAttrsString));
  LM_TMP(("FWD: protocol for forwarding:  %s", protocol));
  LM_TMP(("FWD: host for forwarding:      %s", host));
  LM_TMP(("FWD: port for forwarding:      %d", port));
  LM_TMP(("FWD: format:                   %s", format));
  LM_TMP(("FWD: @context:                 %s", (contextP == NULL)? "Core" : (contextP->type == KjString)? contextP->value.s : kjValueType(contextP->type)));

  int   size    = 256 + strlen(newUriParamAttrsString);
  char* urlPath = (char*) kaAlloc(&orionldState.kalloc, size);

  if (newUriParamAttrsString != NULL)
    snprintf(urlPath, size, "/ngsi-ld/v1/entities/%s?attrs=%s", entityId, newUriParamAttrsString);
  else
    snprintf(urlPath, size, "/ngsi-ld/v1/entities/%s", entityId);

  LM_TMP(("FWD: urlPath: %s", urlPath));

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

  LM_TMP(("FWD: Calling orionldRequestSend, to send the forwarded request"));
  reqOk = orionldRequestSend(&orionldState.httpResponse, protocol, host, port, urlPath, 5000, &detail, &tryAgain, &downloadFailed, "Accept: application/json");

  if (reqOk)
  {
    LM_TMP(("FWD: Parsing the response into a KjNode tree (%s)", orionldState.httpResponse.buf));
    entityP = kjParse(orionldState.kjsonP, orionldState.httpResponse.buf);
  }
  else
    LM_TMP(("FWD: orionldRequestSend failed: %s", detail));

  return entityP;
}



// -----------------------------------------------------------------------------
//
// orionldForwardGetEntity -
//
static KjNode* orionldForwardGetEntity(ConnectionInfo* ciP, char* entityId, KjNode* responseP)
{
  LM_TMP(("FWD: looking up registrations for entity '%s'", entityId));

  KjNode*  regArrayP = mongoCppLegacyRegistrationLookup(entityId);

  if (regArrayP == NULL)
  {
    LM_TMP(("FWD: no registration found for entity '%s'", entityId));
    return NULL;
  }

  LM_TMP(("FWD: found registrations for entity '%s'", entityId));

  //
  // If URI param 'attrs' was used, split the attr-names into an array and expanmd according to @context
  //
  char* uriParamAttrsV[100];
  int   uriParamAttrs = 0;

  if (!ciP->uriParam["attrs"].empty())
  {
    char* uriParamAttrsString = (char*) ciP->uriParam["attrs"].c_str();
    LM_TMP(("FWD: Got an 'attrs' URI param: %s (%s)", uriParamAttrsString, orionldState.uriParams.attrs));

    uriParamAttrs = kStringSplit(uriParamAttrsString, ',', uriParamAttrsV, K_VEC_SIZE(uriParamAttrsV));
    LM_TMP(("FWD: Split the 'attrs' URI param string into an array of %d items", uriParamAttrs));

    //
    // Populate the array uriParamAttrsV with the expanded attribute names
    //
    for (int ix = 0; ix < uriParamAttrs; ix++)
    {
      LM_TMP(("FWD: Expanding attr %d '%s'", ix, uriParamAttrsV[ix]));
      uriParamAttrsV[ix] = orionldContextItemExpand(orionldState.contextP, uriParamAttrsV[ix], NULL, true, NULL);
      LM_TMP(("FWD: Expanded attr %d '%s'", ix, uriParamAttrsV[ix]));
    }
  }

  //
  // Treating all hits from the registrations
  //
  for (KjNode* regP = regArrayP->value.firstChildP; regP != NULL; regP = regP->next)
  {
    KjNode* partTree = orionldForwardGetEntity2(regP, entityId, uriParamAttrsV, uriParamAttrs);

    if (partTree != NULL)
    {
      LM_TMP(("FWD: Got a partTree - now add its attrs to the response"));
      // Move all attributes from 'partTree' into responseP
      KjNode* nodeP = partTree->value.firstChildP;
      KjNode* next;

      while (nodeP != NULL)
      {
        next = nodeP->next;
        LM_TMP(("FWD: Got partTree attr '%s'", nodeP->name));
        if (SCOMPARE3(nodeP->name, 'i', 'd', 0))
        {}
        else if (SCOMPARE5(nodeP->name, 't', 'y', 'p', 'e', 0))
        {}
        else
        {
          LM_TMP(("FWD: Adding '%s' to responseP", nodeP->name));
          kjChildAdd(responseP, nodeP);
        }

        nodeP = next;
      }
    }
  }

  return responseP;
}



// ----------------------------------------------------------------------------
//
// orionldGetEntity -
//
// URI params:
// - attrs               (orionldState.uriParams.attrs)
// - options=keyValues   ()
//
bool orionldGetEntity(ConnectionInfo* ciP)
{
  if (orionldState.uriParams.attrs != NULL)
    LM_TMP(("FWD: Got an 'attrs' URI param: %s", orionldState.uriParams.attrs));

  bool                  keyValues = ciP->uriParamOptions[OPT_KEY_VALUES];
  QueryContextRequest   request;
  QueryContextResponse  response;
  EntityId              entityId(orionldState.wildcard[0], "", "false", false);
  char*                 details;

  LM_T(LmtServiceRoutine, ("In orionldGetEntity: %s", orionldState.wildcard[0]));

  request.entityIdVector.push_back(&entityId);

  //
  // Make sure the ID (orionldState.wildcard[0]) is a valid URI
  //
  if ((urlCheck(orionldState.wildcard[0], &details) == false) && (urnCheck(orionldState.wildcard[0], &details) == false))
  {
    LM_W(("Bad Input (Invalid Entity ID - Not a URL nor a URN)"));
    orionldErrorResponseCreate(OrionldBadRequestData, "Invalid Entity ID", "Not a URL nor a URN");
    return false;
  }

  //
  // FIXME: mongoQueryContext should respond with a KJson tree -
  //        next year perhaps, and when starting with new mongo driver
  //
  ciP->httpStatusCode = mongoQueryContext(&request,
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

  // Create response by converting "QueryContextResponse response" into a KJson tree
  orionldState.responseTree = kjTreeFromQueryContextResponse(ciP, true, orionldState.uriParams.attrs, keyValues, &response);

  if (orionldState.responseTree == NULL)
  {
    // If found in registrations ...
    ciP->httpStatusCode = SccContextElementNotFound;
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
  orionldForwardGetEntity(ciP, orionldState.wildcard[0], orionldState.responseTree);


  // request.entityIdVector.vec.erase(0);  // Remove 'entityId' from entityIdVector

  return true;
}
