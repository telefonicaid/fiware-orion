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
#include "orionld/context/orionldContextTreat.h"                 // orionldContextTreat
#include "orionld/context/orionldUriExpand.h"                    // orionldUriExpand
#include "orionld/serviceRoutines/orionldPostEntity.h"           // Own Interface



// ----------------------------------------------------------------------------
//
// orionldPartialUpdateResponseCreate -
//
void orionldPartialUpdateResponseCreate(ConnectionInfo* ciP)
{
  LM_TMP(("PART-UPDATE: the erroneous attrs are: '%s'", orionldState.errorAttributeArrayP));

  //
  // Rob the incoming Request Tree - performance to be won!
  //
  LM_TMP(("PART-UPDATE: Here"));
  ciP->responseTree = ciP->requestTree;
  LM_TMP(("PART-UPDATE: Here"));
  ciP->requestTree  = NULL;
  LM_TMP(("PART-UPDATE: Here"));

  //
  // For all attrs in ciP->responseTree, remove those that are found in orionldState.errorAttributeArrayP.
  // Remember, the format of orionldState.errorAttributeArrayP is:
  //
  //   |attrName|attrName|attrName|...
  //

  // <TMP>
  int ix = 1;
  for (KjNode* attrNodeP = ciP->responseTree->value.firstChildP; attrNodeP != NULL; attrNodeP = attrNodeP->next)
  {
    LM_TMP(("PART-UPDATE: Attr %d: '%s'", ix, attrNodeP->name));
    ++ix;
  }
  // </TMP>

  KjNode* attrNodeP = ciP->responseTree->value.firstChildP;

  while (attrNodeP != NULL)
  {
    char*   match;
    KjNode* next = attrNodeP->next;
    bool    moved = false;

    LM_TMP(("PART-UPDATE: calling match for '%s'", attrNodeP->name));
    if ((match = strstr(orionldState.errorAttributeArrayP, attrNodeP->name)) != NULL)
    {
      if ((match[-1] == '|') && (match[strlen(attrNodeP->name)] == '|'))
      {
        LM_TMP(("PART-UPDATE: removing child '%s'", attrNodeP->name));
        kjChildRemove(ciP->responseTree, attrNodeP);
        attrNodeP = next;
        moved = true;
      }
    }

    if (moved == false)
      attrNodeP = attrNodeP->next;
  }

  // <TMP>
  for (KjNode* attrNodeP = ciP->responseTree->value.firstChildP; attrNodeP != NULL; attrNodeP = attrNodeP->next)
  {
    LM_TMP(("PART-UPDATE: Attr %d: '%s'", ix, attrNodeP->name));
    ++ix;
  }
  // </TMP>

  LM_TMP(("PART-UPDATE: Created"));
}



// ----------------------------------------------------------------------------
//
// orionldPostEntity -
//
bool orionldPostEntity(ConnectionInfo* ciP)
{
  // 1. Check that the entity exists
  if (mongoEntityExists(orionldState.wildcard[0], orionldState.tenant) == false)
  {
    ciP->httpStatusCode = SccNotFound;
    orionldErrorResponseCreate(ciP, OrionldBadRequestData, "Entity does not exist", orionldState.wildcard[0], OrionldDetailsString);
    return false;
  }

  // Is the payload empty?
  if (ciP->requestTree == NULL)
  {
    ciP->httpStatusCode = SccBadRequest;
    orionldErrorResponseCreate(ciP, OrionldBadRequestData, "No payload", NULL, OrionldDetailsString);
    return false;
  }

  // Is the payload not a JSON object?
  OBJECT_CHECK(ciP->requestTree, kjValueType(ciP->requestTree->type));

  // Is the payload an empty object?
  if (ciP->requestTree->value.firstChildP == NULL)
  {
    ciP->httpStatusCode = SccBadRequest;
    orionldErrorResponseCreate(ciP, OrionldBadRequestData, "Payload is an empty JSON object", NULL, OrionldDetailsString);
    return false;
  }


  UpdateContextRequest   mongoRequest;
  UpdateContextResponse  mongoResponse;
  ContextElement*        ceP       = new ContextElement();  // FIXME: Any way I can avoid to allocate ?
  EntityId*              entityIdP;

  mongoRequest.contextElementVector.push_back(ceP);
  entityIdP = &mongoRequest.contextElementVector[0]->entityId;

  if (orionldState.uriParamOptions.noOverwrite == true)
  {
    LM_TMP(("AppendAttributes: mongoRequest.updateActionType = ActionTypeAppendStrict (as 'noOverwrite' was set)"));
    mongoRequest.updateActionType = ActionTypeAppendStrict;
  }
  else
  {
    LM_TMP(("AppendAttributes: mongoRequest.updateActionType = ActionTypeAppend (as 'noOverwrite' was NOT set)"));
    mongoRequest.updateActionType = ActionTypeAppend;
  }

  entityIdP->id = orionldState.wildcard[0];


  //
  // 1. Lookup special nodes and actuate over them
  //   FIXME: Why not remove them from the tree? Only Properties/Relationships left after that ...
  //
  KjNode* contextNodeP = NULL;

  for (KjNode* kNodeP = ciP->requestTree->value.firstChildP; kNodeP != NULL; kNodeP = kNodeP->next)
  {
    if (SCOMPARE9(kNodeP->name, '@', 'c', 'o', 'n', 't', 'e', 'x', 't', 0))
    {
      contextNodeP = kNodeP;

      if (orionldContextTreat(ciP, contextNodeP, (char*) entityIdP->id.c_str(), NULL) == false)
      {
        // Error payload set by orionldContextTreat
        mongoRequest.release();
        return false;
      }

      // FIXME: Remove the @context node from the tree - to avoid 'if (kNodeP != contextNodeP)' in the following loop
      break;
    }
  }

  // 3. Iterate over the object, to get all attributes
  for (KjNode* kNodeP = ciP->requestTree->value.firstChildP; kNodeP != NULL; kNodeP = kNodeP->next)
  {
    KjNode* attrTypeNodeP = NULL;

    if (kNodeP != contextNodeP)
    {
      ContextAttribute* caP = new ContextAttribute();

      // FIXME: Mode attributeTreat to separate file
      extern bool attributeTreat(ConnectionInfo* ciP, KjNode* kNodeP, ContextAttribute* caP, KjNode** typeNodePP);
      if (attributeTreat(ciP, kNodeP, caP, &attrTypeNodeP) == false)
      {
        mongoRequest.release();
        ciP->httpStatusCode = SccBadRequest;  // FIXME: Should be set inside 'attributeTreat' - could be 500, not 400 ...
        LM_E(("attributeTreat failed"));
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
          mongoRequest.release();
          orionldErrorResponseCreate(ciP, OrionldBadRequestData, details, kNodeP->name, OrionldDetailsAttribute);
          return false;
        }

        caP->name = longName;
      }

      // NO URI Expansion for Attribute TYPE
      caP->type = attrTypeNodeP->value.s;

      // Add the attribute to the attr vector
      ceP->contextAttributeVector.push_back(caP);
    }
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
      orionldErrorResponseCreate(ciP, OrionldBadRequestData, "Internal Error", "Error from mongo backend", OrionldDetailsString);
    }

    retValue = false;
  }

  mongoRequest.release();
  mongoResponse.release();

  return retValue;
}
