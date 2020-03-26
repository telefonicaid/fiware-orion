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
#include <string>
#include <vector>

extern "C"
{
#include "kbase/kStringSplit.h"                                // kStringSplit
#include "kjson/kjBuilder.h"                                   // kjArray, kjChildAdd, ...
}

#include "logMsg/logMsg.h"                                     // LM_*
#include "logMsg/traceLevels.h"                                // Lmt*

#include "rest/ConnectionInfo.h"                               // ConnectionInfo
#include "ngsi10/QueryContextRequest.h"                        // QueryContextRequest
#include "ngsi10/QueryContextResponse.h"                       // QueryContextResponse
#include "mongoBackend/mongoQueryContext.h"                    // mongoQueryContext

#include "orionld/common/SCOMPARE.h"                           // SCOMPAREx
#include "orionld/common/urlCheck.h"                           // urlCheck
#include "orionld/common/urnCheck.h"                           // urnCheck
#include "orionld/common/qLex.h"                               // qLex
#include "orionld/common/qParse.h"                             // qParse
#include "orionld/common/qTreeToBsonObj.h"                     // qTreeToBsonObj
#include "orionld/common/orionldState.h"                       // orionldState
#include "orionld/kjTree/kjTreeFromQueryContextResponse.h"     // kjTreeFromQueryContextResponse
#include "orionld/context/orionldCoreContext.h"                // orionldDefaultUrl
#include "orionld/common/orionldErrorResponse.h"               // orionldErrorResponseCreate
#include "orionld/context/orionldContextItemExpand.h"          // orionldContextItemExpand
#include "orionld/serviceRoutines/orionldGetEntities.h"        // Own Interface



// ----------------------------------------------------------------------------
//
// orionldGetEntities -
//
// URI params:
// - id
// - idPattern
// - type         (can't point to NULL as its converted to a std::string)
// - typePattern  (not possible - ignored (need an exact type name to lookup alias))
// - q
// - attrs
// - mq          - Not used in ngsi-ld. []/. is used instead of q/mq
// - geometry
// - coordinates
// - georel
// - maxDistance
// - options=keyValues
//
bool orionldGetEntities(ConnectionInfo* ciP)
{
  char*                 id             = (ciP->uriParam["id"].empty())?          NULL : (char*) ciP->uriParam["id"].c_str();
  char*                 type           = (ciP->uriParam["type"].empty())?        (char*) "" : (char*) ciP->uriParam["type"].c_str();
  char*                 idPattern      = (ciP->uriParam["idPattern"].empty())?   NULL : (char*) ciP->uriParam["idPattern"].c_str();
  char*                 q              = (ciP->uriParam["q"].empty())?           NULL : (char*) ciP->uriParam["q"].c_str();
  char*                 attrs          = (ciP->uriParam["attrs"].empty())?       NULL : (char*) ciP->uriParam["attrs"].c_str();

  char*                 geometry       = (ciP->uriParam["geometry"].empty())?    NULL : (char*) ciP->uriParam["geometry"].c_str();
  char*                 georel         = (ciP->uriParam["georel"].empty())?      NULL : (char*) ciP->uriParam["georel"].c_str();
  char*                 coordinates    = (ciP->uriParam["coordinates"].empty())? NULL : (char*) ciP->uriParam["coordinates"].c_str();

  char*                 idString       = (id != NULL)? id      : idPattern;
  const char*           isIdPattern    = (id != NULL)? "false" : "true";
  bool                  isTypePattern  = (*type != 0)? false   : true;
  EntityId*             entityIdP;
  char*                 typeExpanded   = NULL;
  char*                 detail;
  char*                 idVector[32];    // Is 32 a good limit?
  char*                 typeVector[32];  // Is 32 a good limit?
  int                   idVecItems     = (int) sizeof(idVector) / sizeof(idVector[0]);
  int                   typeVecItems   = (int) sizeof(typeVector) / sizeof(typeVector[0]);
  bool                  keyValues      = ciP->uriParamOptions[OPT_KEY_VALUES];
  QueryContextRequest   mongoRequest;
  QueryContextResponse  mongoResponse;

  if ((id == NULL) && (idPattern == NULL) && (*type == 0) && ((geometry == NULL) || (*geometry == 0)) && (attrs == NULL) && (q == NULL))
  {
    LM_W(("Bad Input (too broad query - need at least one of: entity-id, entity-type, geo-location, attribute-list, Q-filter"));

    orionldErrorResponseCreate(OrionldBadRequestData,
                               "too broad query",
                               "need at least one of: entity-id, entity-type, geo-location, attribute-list, Q-filter");

    orionldState.httpStatusCode = SccBadRequest;
    return false;
  }

  if ((idPattern != NULL) && (id != NULL))
  {
    LM_W(("Bad Input (both 'idPattern' and 'id' used)"));
    orionldErrorResponseCreate(OrionldBadRequestData, "Incompatible parameters", "id, idPattern");
    orionldState.httpStatusCode = SccBadRequest;
    return false;
  }

  //
  // If 'georel' is present, make sure it has a valid value
  //

  if (georel != NULL)
  {
    if ((strncmp(georel, "near", 4)        != 0) &&
        (strncmp(georel, "within", 6)      != 0) &&
        (strncmp(georel, "contains", 8)    != 0) &&
        (strncmp(georel, "overlaps", 8)    != 0) &&
        (strncmp(georel, "intersects", 10) != 0) &&
        (strncmp(georel, "equals", 6)      != 0) &&
        (strncmp(georel, "disjoint", 8)    != 0))
    {
      LM_W(("Bad Input (invalid value for georel)"));
      orionldErrorResponseCreate(OrionldBadRequestData, "invalid value for georel", georel);
      orionldState.httpStatusCode = SccBadRequest;
      return false;
    }

    char* georelExtra = strstr(georel, ";");

    if (georelExtra != NULL)
    {
      ++georelExtra;  // Step over ';', but don't "destroy" the string - it is used as is later on

      if ((strncmp(georelExtra, "minDistance==", 11) != 0) && (strncmp(georelExtra, "maxDistance==", 11) != 0))
      {
        LM_W(("Bad Input (invalid value for georel parameter: %s)", georelExtra));
        orionldErrorResponseCreate(OrionldBadRequestData, "invalid value for georel parameter", georel);
        orionldState.httpStatusCode = SccBadRequest;
        return false;
      }
    }
  }

  if (geometry != NULL)
  {
    if (coordinates == NULL)
    {
      LM_W(("Bad Input (coordinates missing)"));

      orionldErrorResponseCreate(OrionldBadRequestData,
                                 "no coordinates",
                                 "geometry without coordinates");

      orionldState.httpStatusCode = SccBadRequest;
      return false;
    }

    if (georel == NULL)
    {
      LM_W(("Bad Input (georel missing)"));

      orionldErrorResponseCreate(OrionldBadRequestData,
                                 "no georel",
                                 "geometry with coordinates but without georel");

      orionldState.httpStatusCode = SccBadRequest;
      return false;
    }

    Scope*       scopeP = new Scope(SCOPE_TYPE_LOCATION, "");
    std::string  errorString;

    //
    // In APIv2, the vector is a string without [], in NGSI-LD, [] are present. Must remove ...
    //
    if (coordinates[0] == '[')
    {
      ++coordinates;

      int len = strlen(coordinates);
      if (coordinates[len - 1] == ']')
        coordinates[len - 1] = 0;
    }

    if (scopeP->fill(ciP->apiVersion, geometry, coordinates, georel, &errorString) != 0)
    {
      scopeP->release();
      delete scopeP;

      LM_E(("Geo: Scope::fill failed"));
      orionldErrorResponseCreate(OrionldInternalError, "error filling a scope", errorString.c_str());
      orionldState.httpStatusCode = SccBadRequest;
      return false;
    }

    LM_E(("Geo: Scope::fill OK"));
    mongoRequest.restriction.scopeVector.push_back(scopeP);
  }

  if (idString == NULL)
  {
    idString    = (char*) ".*";
    isIdPattern = (char*) "true";
  }

  if (*type == 0)  // No type given - match all types
  {
    type          = (char*) ".*";
    isTypePattern = true;
    typeVecItems  = 0;  // Just to avoid entering the "if (typeVecItems == 1)"
  }
  else
    typeVecItems = kStringSplit(type, ',', (char**) typeVector, typeVecItems);

  idVecItems   = kStringSplit(id, ',', (char**) idVector, idVecItems);

  //
  // ID-list and Type-list at the same time is not supported
  //
  if ((idVecItems > 1) && (typeVecItems > 1))
  {
    LM_W(("Bad Input (URI params /id/ and /type/ are both lists - Not Permitted)"));
    orionldErrorResponseCreate(OrionldBadRequestData, "URI params /id/ and /type/ are both lists", "Not Permitted");
    return false;
  }

  //
  // Make sure all IDs are valid URIs
  //
  for (int ix = 0; ix < idVecItems; ix++)
  {
    if ((urlCheck(idVector[ix], &detail) == false) && (urnCheck(idVector[ix], &detail) == false))
    {
      LM_W(("Bad Input (Invalid Entity ID - Not a URL nor a URN)"));
      orionldErrorResponseCreate(OrionldBadRequestData, "Invalid Entity ID", "Not a URL nor a URN");
      return false;
    }
  }

  if (typeVecItems == 1)  // type needs to be modified according to @context
  {
    char* detail;

    // No expansion desired if the type is already a FQN
    if (urlCheck(type, &detail) == false)
      type = orionldContextItemExpand(orionldState.contextP, type, NULL, true, NULL);

    isTypePattern = false;  // Just in case ...
  }

  if (idVecItems > 1)  // A list of Entity IDs
  {
    for (int ix = 0; ix < idVecItems; ix++)
    {
      entityIdP = new EntityId(idVector[ix], type, "false", isTypePattern);
      mongoRequest.entityIdVector.push_back(entityIdP);
    }
  }
  else if (typeVecItems > 1)  // A list of Entity Types
  {
    for (int ix = 0; ix < typeVecItems; ix++)
    {
      if (urlCheck(typeVector[ix], &detail) == false)
        typeExpanded = orionldContextItemExpand(orionldState.contextP, typeVector[ix], NULL, true, NULL);
      else
        typeExpanded = typeVector[ix];

      entityIdP = new EntityId(idString, typeExpanded, isIdPattern, false);
      mongoRequest.entityIdVector.push_back(entityIdP);
    }
  }
  else  // Definitely no lists in EntityId id/type
  {
    entityIdP = new EntityId(idString, type, isIdPattern, isTypePattern);
    mongoRequest.entityIdVector.push_back(entityIdP);
  }

  if (attrs != NULL)
  {
    char* shortNameVector[100];
    int   vecItems = (int) sizeof(shortNameVector) / sizeof(shortNameVector[0]);

    vecItems = kStringSplit(attrs, ',', (char**) shortNameVector, vecItems);

    for (int ix = 0; ix < vecItems; ix++)
    {
      const char* longName = orionldContextItemExpand(orionldState.contextP, shortNameVector[ix], NULL, true, NULL);

      mongoRequest.attributeList.push_back(longName);
    }
  }

  if (q != NULL)
  {
    char*  title;
    char*  detail;
    QNode* lexList;
    QNode* qTree;

    if ((lexList = qLex(q, &title, &detail)) == NULL)
    {
      LM_W(("Bad Input (qLex: %s: %s)", title, detail));
      orionldErrorResponseCreate(OrionldBadRequestData, title, detail);
      mongoRequest.release();
      return false;
    }

    if ((qTree = qParse(lexList, &title, &detail)) == NULL)
    {
      LM_W(("Bad Input (qParse: %s: %s)", title, detail));
      orionldErrorResponseCreate(OrionldBadRequestData, title, detail);
      mongoRequest.release();
      return false;
    }


    //
    // FIXME: this part about Q-Filter depends on the database and must be moved to
    //        the DB layer
    //
    orionldState.qMongoFilterP = new mongo::BSONObj;

    mongo::BSONObjBuilder objBuilder;
    if (qTreeToBsonObj(qTree, &objBuilder, &title, &detail) == false)
    {
      LM_W(("Bad Input (qTreeToBsonObj: %s: %s)", title, detail));
      orionldErrorResponseCreate(OrionldBadRequestData, title, detail);
      mongoRequest.release();
      return false;
    }

    *orionldState.qMongoFilterP = objBuilder.obj();
  }


  //
  // Call mongoBackend
  //
  long long   count;
  long long*  countP = (ciP->uriParamOptions["count"] == true)? &count : NULL;

  orionldState.httpStatusCode = mongoQueryContext(&mongoRequest,
                                                  &mongoResponse,
                                                  orionldState.tenant,
                                                  ciP->servicePathV,
                                                  ciP->uriParam,
                                                  ciP->uriParamOptions,
                                                  countP,
                                                  ciP->apiVersion);


  //
  // Transform QueryContextResponse to KJ-Tree
  //
  orionldState.httpStatusCode = SccOk;  // FIXME: What about the response from mongoQueryContext???

  orionldState.responseTree = kjTreeFromQueryContextResponse(ciP, false, NULL, keyValues, &mongoResponse);

  // Add "count" if asked for
  if (countP != NULL)
  {
    char cV[32];
    snprintf(cV, sizeof(cV), "%llu", *countP);
    ciP->httpHeader.push_back(HTTP_FIWARE_TOTAL_COUNT);
    ciP->httpHeaderValue.push_back(cV);
  }

  return true;
}
