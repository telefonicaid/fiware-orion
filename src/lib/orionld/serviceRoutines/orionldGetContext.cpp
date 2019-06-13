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
#include "kjson/KjNode.h"                                        // KjNode
#include "kjson/kjBuilder.h"                                     // kjObject, kjString, kjBoolean, ...
}

#include "logMsg/logMsg.h"                                       // LM_*
#include "logMsg/traceLevels.h"                                  // Lmt*

#include "rest/ConnectionInfo.h"                                 // ConnectionInfo
#include "mongoBackend/mongoQueryContext.h"                      // mongoQueryContext
#include "orionld/common/orionldErrorResponse.h"                 // orionldErrorResponseCreate
#include "orionld/common/OrionldConnection.h"                    // orionldState
#include "orionld/context/orionldContextLookup.h"                // orionldContextLookup
#include "orionld/kjTree/kjTreeFromContextContextAttribute.h"    // kjTreeFromContextContextAttribute
#include "orionld/serviceRoutines/orionldGetContext.h"           // Own Interface



// ----------------------------------------------------------------------------
//
// orionldGetContext -
//
bool orionldGetContext(ConnectionInfo* ciP)
{
  LM_T(LmtServiceRoutine, ("In orionldGetContext - looking up context '%s'", orionldState.wildcard[0]));

  OrionldContext* contextP    = orionldContextLookup(orionldState.wildcard[0]);
  KjNode*         contextTree = NULL;

  orionldState.useLinkHeader = false;  // We don't want the Link header for context requests

  if (contextP != NULL)
  {
    contextTree = contextP->tree;
  }
  else
  {
    // OK, might be an entity context then ...

    QueryContextRequest   request;
    EntityId              entityId(orionldState.wildcard[0], "", "false", false);
    QueryContextResponse  response;

    request.entityIdVector.push_back(&entityId);

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
      orionldErrorResponseCreate(ciP, OrionldBadRequestData, "Bad Request", NULL, OrionldDetailsString);
      return false;
    }
    else if (response.contextElementResponseVector.size() > 0)
    {
      ContextAttribute* contextAttributeP = response.contextElementResponseVector[0]->contextElement.contextAttributeVector.lookup("@context");

      if (contextAttributeP != NULL)
      {
        char* details;

        contextTree = kjTreeFromContextContextAttribute(ciP, contextAttributeP, &details);
      }
    }

    if (contextTree == NULL)
    {
      orionldErrorResponseCreate(ciP, OrionldBadRequestData, "Context Not Found", orionldState.wildcard[0], OrionldDetailsString);
      ciP->httpStatusCode = SccContextElementNotFound;
      return false;
    }
  }

  ciP->responseTree = kjObject(orionldState.kjsonP, "@context");
  // NOTE: No need to free - as the kjObject is allocated in "orionldState.kjsonP", it gets deleted when the thread is reused.

  if (ciP->responseTree == NULL)
  {
    orionldErrorResponseCreate(ciP, OrionldBadRequestData, "kjObject failed", "out of memory?", OrionldDetailsString);
    ciP->httpStatusCode = SccReceiverInternalError;
    return false;
  }

  ciP->responseTree->value.firstChildP = contextTree;

  return true;
}
