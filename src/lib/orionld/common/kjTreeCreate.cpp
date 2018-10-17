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
#include "kjson/KjNode.h"                                      // KjNode
#include "kjson/kjBuilder.h"                                   // kjObject, kjString, kjBoolean, ...
}

#include "logMsg/logMsg.h"                                     // LM_*
#include "logMsg/traceLevels.h"                                // Lmt*

#include "rest/ConnectionInfo.h"                               // ConnectionInfo
#include "ngsi10/QueryContextResponse.h"                       // QueryContextResponse
#include "orionld/common/orionldErrorResponse.h"               // OrionldResponseErrorType, orionldErrorResponse
#include "orionld/context/orionldCoreContext.h"                // orionldCoreContext
#include "orionld/common/kjTreeCreate.h"                       // Own interface



// -----------------------------------------------------------------------------
//
// httpStatusCodeToOrionldErrorType
//
static OrionldResponseErrorType httpStatusCodeToOrionldErrorType(HttpStatusCode sc)
{
  switch (sc)
  {
  case SccNone:                           return OrionldInternalError;
  case SccOk:                             return OrionldInternalError;  // Should not be here if 200 ...
  case SccCreated:                        return OrionldInternalError;
  case SccNoContent:                      return OrionldInternalError;
  case SccBadRequest:                     return OrionldBadRequestData;
  case SccForbidden:                      return OrionldOperationNotSupported;
  case SccContextElementNotFound:         return OrionldResourceNotFound;
  case SccBadVerb:                        return OrionldInvalidRequest;
  case SccNotAcceptable:                  return OrionldInvalidRequest;
  case SccConflict:                       return OrionldAlreadyExists;
  case SccContentLengthRequired:          return OrionldInvalidRequest;
  case SccRequestEntityTooLarge:          return OrionldInvalidRequest;
  case SccUnsupportedMediaType:           return OrionldInvalidRequest;
  case SccInvalidModification:            return OrionldOperationNotSupported;
  case SccSubscriptionIdNotFound:         return OrionldResourceNotFound;
  case SccMissingParameter:               return OrionldBadRequestData;
  case SccInvalidParameter:               return OrionldBadRequestData;
  case SccErrorInMetadata:                return OrionldBadRequestData;
  case SccEntityIdReNotAllowed:           return OrionldBadRequestData;
  case SccEntityTypeRequired:             return OrionldBadRequestData;
  case SccAttributeListRequired:          return OrionldInvalidRequest;
  case SccReceiverInternalError:          return OrionldInternalError;
  case SccNotImplemented:                 return OrionldOperationNotSupported;
  }

  return OrionldInternalError;
}



// -----------------------------------------------------------------------------
//
// kjTreeCreateFromQueryContextResponse -
//
KjNode* kjTreeCreateFromQueryContextResponse(ConnectionInfo* ciP, QueryContextResponse* responseP)
{
  //
  // Error?
  //
  if (responseP->errorCode.code == SccNone)
    responseP->errorCode.code = SccOk;

  if (responseP->errorCode.code != SccOk)
  {
    LM_TMP(("Error %d from mongoBackend", responseP->errorCode.code));
    OrionldResponseErrorType errorType = httpStatusCodeToOrionldErrorType(responseP->errorCode.code);
    
    orionldErrorResponseCreate(ciP, errorType, responseP->errorCode.reasonPhrase.c_str(), responseP->errorCode.details.c_str(), OrionldDetailsString);

    if (responseP->errorCode.code == SccContextElementNotFound)
      ciP->httpStatusCode = responseP->errorCode.code;

    return ciP->responseTree;    
  }

  int hits  = responseP->contextElementResponseVector.size();

  if (hits == 0)  // No hit
  {
    ciP->responseTree = NULL;
    return NULL;
  }
  else if (hits > 1)  // More than one hit - not possible!
  {
    orionldErrorResponseCreate(ciP, OrionldInternalError, "More than one hit", ciP->wildcard[0], OrionldDetailsEntity);
  }


  KjNode*           top = kjObject(NULL, NULL);
  ContextElement*   ceP = &responseP->contextElementResponseVector[0]->contextElement;
  KjNode*           nodeP;

  //
  // id
  //
  nodeP = kjString(ciP->kjsonP, "id", ceP->entityId.id.c_str());
  kjChildAdd(top, nodeP);


  //
  // type
  //
  if (ceP->entityId.type != "")
  {
    nodeP = kjString(ciP->kjsonP, "type", ceP->entityId.type.c_str());
    kjChildAdd(top, nodeP);
  }


  //
  // Attributes, including @context
  //
  ContextAttribute* contextP = NULL;

  for (unsigned int aIx = 0; aIx < ceP->contextAttributeVector.size(); aIx++)
  {
    ContextAttribute* aP = ceP->contextAttributeVector[aIx];
    
    if (strcmp(aP->name.c_str(), "@context") == 0)
    {
      contextP = aP;
      continue;
    }

    KjNode* aTop = kjObject(ciP->kjsonP, aP->name.c_str());

    // type
    if (aP->type != "")
    {
      nodeP = kjString(ciP->kjsonP, "type", aP->type.c_str());
      kjChildAdd(aTop, nodeP);
    }

    // value
    switch (aP->valueType)
    {
    case orion::ValueTypeString:
      nodeP = kjString(ciP->kjsonP, "value", aP->stringValue.c_str());
      kjChildAdd(aTop, nodeP);
      break;

    case orion::ValueTypeNumber:
      nodeP = kjFloat(ciP->kjsonP, "value", aP->numberValue);  // FIXME: kjInteger or kjFloat ...
      kjChildAdd(aTop, nodeP);
      break;

    case orion::ValueTypeBoolean:
      nodeP = kjBoolean(ciP->kjsonP, "value", (KBool) aP->boolValue);
      kjChildAdd(aTop, nodeP);
      break;

    case orion::ValueTypeNull:
      nodeP = kjNull(ciP->kjsonP, "value");
      kjChildAdd(aTop, nodeP);
      break;

    case orion::ValueTypeNotGiven:
      nodeP = kjString(ciP->kjsonP, "value", "UNKNOWN TYPE");
      kjChildAdd(aTop, nodeP);
      break;

    case orion::ValueTypeVector:
    case orion::ValueTypeObject:
      nodeP = kjString(ciP->kjsonP, "value", "compounds for later ...");
      kjChildAdd(aTop, nodeP);
      break;
    }

    kjChildAdd(top, aTop);  // Adding the attribute to the tree
  }


  //
  // If no context inside attribute list, then the default context has been used
  //
  if (contextP == NULL)
  {
    nodeP = kjString(ciP->kjsonP, "@context", orionldCoreContext.url);
    kjChildAdd(top, nodeP);
  }
  else
  {
    switch (contextP->valueType)
    {
    case orion::ValueTypeString:
      nodeP = kjString(ciP->kjsonP, "@context", contextP->stringValue.c_str());
      kjChildAdd(top, nodeP);
      break;

    case orion::ValueTypeVector:
      nodeP = kjArray(ciP->kjsonP, "@context");
      kjChildAdd(top, nodeP);

      for (unsigned int ix = 0; ix < contextP->compoundValueP->childV.size(); ix++)
      {
        orion::CompoundValueNode*  compoundP     = contextP->compoundValueP->childV[ix];
        KjNode*                    contextItemP  = kjString(ciP->kjsonP, NULL, compoundP->stringValue.c_str());
        kjChildAdd(nodeP, contextItemP);
      }
      break;

    default:
      orionldErrorResponseCreate(ciP, OrionldInternalError, "invalid context", "not a string nor an array", OrionldDetailsString);
      // FIXME: leaks!!! (Call kjFree(top))?
      return ciP->responseTree;
    }
  }

  return top;
}
