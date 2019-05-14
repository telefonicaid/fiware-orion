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
extern "C"
{
#include "kbase/kStringSplit.h"                                // kStringSplit
}

#include "logMsg/logMsg.h"                                     // LM_*
#include "logMsg/traceLevels.h"                                // Lmt*

#include "rest/ConnectionInfo.h"                               // ConnectionInfo
#include "mongoBackend/mongoQueryContext.h"                    // mongoQueryContext

#include "orionld/common/urlCheck.h"                           // urlCheck
#include "orionld/common/urnCheck.h"                           // urnCheck
#include "orionld/context/orionldContextAdd.h"                 // Add a context to the context list
#include "orionld/kjTree/kjTreeFromQueryContextResponse.h"     // kjTreeFromQueryContextResponse
#include "orionld/kjTree/kjTreeFromQueryContextResponseWithAttrList.h"     // kjTreeFromQueryContextResponseWithAttrList
#include "orionld/common/orionldErrorResponse.h"               // orionldErrorResponseCreate
#include "orionld/context/orionldUriExpand.h"                  // orionldUriExpand
#include "orionld/serviceRoutines/orionldGetEntity.h"          // Own Interface



// ----------------------------------------------------------------------------
//
// orionldGetEntity -
//
// URI params:
// - attrs
// - options=keyValues
//
bool orionldGetEntity(ConnectionInfo* ciP)
{
  char*                 attrs     = (ciP->uriParam["attrs"].empty())? NULL : (char*) ciP->uriParam["attrs"].c_str();
  bool                  keyValues = ciP->uriParamOptions[OPT_KEY_VALUES];
  QueryContextRequest   request;
  QueryContextResponse  response;
  EntityId              entityId(ciP->wildcard[0], "", "false", false);
  char*                 details;

  LM_T(LmtServiceRoutine, ("In orionldGetEntity: %s", ciP->wildcard[0]));

  request.entityIdVector.push_back(&entityId);

  //
  // Make sure the ID (ciP->wildcard[0]) is a valid URI
  //
  if ((urlCheck(ciP->wildcard[0], &details) == false) && (urnCheck(ciP->wildcard[0], &details) == false))
  {
    LM_W(("Bad Input (Invalid Entity ID - Not a URL nor a URN)"));
    orionldErrorResponseCreate(ciP, OrionldBadRequestData, "Invalid Entity ID", "Not a URL nor a URN", OrionldDetailsString);
    return false;
  }


  //
  // FIXME: mongoQueryContext should respond with a KJson tree -
  //        next year perhaps, and when starting with new mongo driver
  //
  ciP->httpStatusCode = mongoQueryContext(&request,
                                          &response,
                                          ciP->httpHeaders.tenant,
                                          ciP->servicePathV,
                                          ciP->uriParam,
                                          ciP->uriParamOptions,
                                          NULL,
                                          ciP->apiVersion);

  if (response.errorCode.code == SccBadRequest)
  {
    orionldErrorResponseCreate(ciP, OrionldBadRequestData, "Bad Request", NULL, OrionldDetailsString);
    return false;
  }

  //
  // Create response by converting "QueryContextResponse response" into a KJson tree
  // But first, check for "404 Not Found"
  //
  if (attrs != NULL)
  {
    char  longName[256];
    char* details;
    char* shortName;
    char* shortNameVector[32];
    int   vecItems = (int) sizeof(shortNameVector) / sizeof(shortNameVector[0]);;

    vecItems = kStringSplit(attrs, ',', (char**) shortNameVector, vecItems);

    int   attrListLen = 1024;
    char* attrList    = (char*) malloc(attrListLen);
    char* attrListEnd = &attrList[1];

    attrList[0] = ',';
    attrList[1] = 0;

    for (int ix = 0; ix < vecItems; ix++)
    {
      shortName = shortNameVector[ix];

      if (orionldUriExpand(ciP->contextP, shortName, longName, sizeof(longName), &details) == true)
      {
        int       len  = strlen(longName);
        long long used = (long long) attrListEnd - (long long) attrList;

        if (used + len > attrListLen + 1)
        {
          attrListLen += 1024;
          attrList     = (char*) realloc(attrList, attrListLen);

          if (attrList == NULL)
          {
            orionldErrorResponseCreate(ciP, OrionldInternalError, "Out of memory", NULL, OrionldDetailsString);
            return false;
          }

          attrListEnd = &attrList[used];
        }

        strcpy(attrListEnd, longName);
        attrListEnd += len;
        *attrListEnd = ',';
        ++attrListEnd;
      }
      else
      {
        orionldErrorResponseCreate(ciP, OrionldBadRequestData, "Error during URI expansion of attribute", shortName, OrionldDetailsString);
        return false;
      }
    }
    *attrListEnd = 0;

    ciP->responseTree = kjTreeFromQueryContextResponseWithAttrList(ciP, true, attrList, keyValues, &response);
    free(attrList);
  }
  else
    ciP->responseTree = kjTreeFromQueryContextResponse(ciP, true, keyValues, &response);


  if (ciP->responseTree == NULL)
  {
    ciP->httpStatusCode = SccContextElementNotFound;
  }

  // request.entityIdVector.vec.erase(0);  // Remove 'entityId' from entityIdVector

  return true;
}
