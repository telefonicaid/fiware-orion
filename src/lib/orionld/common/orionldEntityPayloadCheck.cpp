/*
*
* Copyright 2019 FIWARE Foundation e.V.
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
* Author: Gabriel Quaresma
*/
#include "logMsg/logMsg.h"                                       // LM_*
#include "logMsg/traceLevels.h"                                  // Lmt*

extern "C"
{
#include "kjson/KjNode.h"                                        // KjNode
#include "kjson/kjBuilder.h"                                     // kjString, kjObject, ...
#include "kjson/kjRender.h"                                      // kjRender
}

#include "common/globals.h"                                      // parse8601Time
#include "rest/ConnectionInfo.h"                                 // ConnectionInfo
#include "rest/httpHeaderAdd.h"                                  // httpHeaderLocationAdd
#include "orionTypes/OrionValueType.h"                           // orion::ValueType
#include "orionTypes/UpdateActionType.h"                         // ActionType
#include "parse/CompoundValueNode.h"                             // CompoundValueNode
#include "ngsi/ContextAttribute.h"                               // ContextAttribute
#include "ngsi10/UpdateContextRequest.h"                         // UpdateContextRequest
#include "ngsi10/UpdateContextResponse.h"                        // UpdateContextResponse
#include "mongoBackend/mongoEntityExists.h"                      // mongoEntityExists
#include "mongoBackend/mongoUpdateContext.h"                     // mongoUpdateContext

#include "orionld/rest/orionldServiceInit.h"                     // orionldHostName, orionldHostNameLen
#include "orionld/common/orionldErrorResponse.h"                 // orionldErrorResponseCreate
#include "orionld/common/SCOMPARE.h"                             // SCOMPAREx
#include "orionld/common/CHECK.h"                                // CHECK
#include "orionld/common/urlCheck.h"                             // urlCheck
#include "orionld/common/urnCheck.h"                             // urnCheck
#include "orionld/common/orionldState.h"                         // orionldState
#include "orionld/common/orionldAttributeTreat.h"                // orionldAttributeTreat
#include "orionld/context/orionldCoreContext.h"                  // orionldDefaultUrl, orionldCoreContext
#include "orionld/context/orionldContextAdd.h"                   // Add a context to the context list
#include "orionld/context/orionldContextLookup.h"                // orionldContextLookup
#include "orionld/context/orionldContextItemLookup.h"            // orionldContextItemLookup
#include "orionld/context/orionldContextList.h"                  // orionldContextHead, orionldContextTail
#include "orionld/context/orionldContextListInsert.h"            // orionldContextListInsert
#include "orionld/context/orionldContextPresent.h"               // orionldContextPresent
#include "orionld/context/orionldUserContextKeyValuesCheck.h"    // orionldUserContextKeyValuesCheck
#include "orionld/common/orionldEntityPayloadCheck.h"            // Own interface



// -----------------------------------------------------------------------------
//
// orionldValidName -
//
bool orionldValidName(char* name, char** detailsPP)
{
  if (name == NULL)
  {
    *detailsPP = (char*) "empty name";
    return false;
  }

  for (; *name != 0; ++name)
  {
    //
    // Valid chars:
    //   o a-z: 97-122
    //   o A-Z: 65-90
    //   o 0-9: 48-57
    //   o '_': 95
    //

    if ((*name >= 'a') && (*name <= 'z'))
      continue;
    if ((*name >= 'A') && (*name <= 'Z'))
      continue;
    if ((*name >= '0') && (*name <= '9'))
      continue;
    if (*name == '_')
      continue;
    if (*name == ':')
      continue;

    *detailsPP = (char*) "invalid character in name";
    return false;
  }

  return true;
}



// -----------------------------------------------------------------------------
//
// checkEntityIdFieldExistis -
//
static bool checkEntityIdFieldExists(void)
{
  if (orionldState.payloadIdNode == NULL)
  {
    orionldErrorResponseCreate(OrionldBadRequestData, "Entity id is missing", "The 'id' field is mandatory");
    return false;
  }

  if (orionldState.payloadTypeNode == NULL)
  {
    const char* entityStartTitle       = "Entity - ";
    const char* idEntityFromPayload    = orionldState.payloadIdNode->value.s;

    char* titleExpanded;
    titleExpanded = (char*) malloc(1 + strlen(entityStartTitle) + strlen(idEntityFromPayload));
    strcpy(titleExpanded, entityStartTitle);
    strcat(titleExpanded, idEntityFromPayload);

    orionldErrorResponseCreate(OrionldBadRequestData, titleExpanded, "The 'type' field is mandatory");
    return false;
  }

  return true;
}



// -----------------------------------------------------------------------------
//
// orionldEntityPayloadCheck -
//
bool orionldEntityPayloadCheck
(
  ConnectionInfo*  ciP,
  KjNode*          kNodeP,
  KjNode**         locationNodePP,
  KjNode**         observationSpaceNodePP,
  KjNode**         operationSpaceNodePP,
  KjNode**         createdAtPP,
  KjNode**         modifiedAtPP,
  bool             isBatchOperation
)
{
  if (isBatchOperation == false)
  {
    OBJECT_CHECK(orionldState.requestTree, "toplevel");
    //
    // Check presence of mandatory fields
    //
    if (checkEntityIdFieldExists() == false) return false;
  }
  char*    detailsP;
  KjNode*  locationNodeP          = NULL;
  KjNode*  observationSpaceNodeP  = NULL;
  KjNode*  operationSpaceNodeP    = NULL;
  KjNode*  createdAtP             = NULL;
  KjNode*  modifiedAtP            = NULL;

  //
  // Check for duplicated items and that data types are correct
  //
  while (kNodeP != NULL)
  {
    if (isBatchOperation == true)
    {
      if (strcmp(kNodeP->name, "id") == 0)
      {
        orionldState.payloadIdNode = kNodeP;

        if (orionldState.payloadIdNode->type != KjString)
        {
          orionldErrorResponseCreate(OrionldBadRequestData, "Invalid Entity Id", "Must be a JSON String");
          return false;
        }

        if ((urlCheck(orionldState.payloadIdNode->value.s, &detailsP) == false) && (urnCheck(orionldState.payloadIdNode->value.s, &detailsP) == false))
        {
          orionldErrorResponseCreate(OrionldBadRequestData, "Invalid Entity id", "The id specified cannot be resolved to a URL or URN");
          return false;
        }
        //
        // If the entity already exists, an error should be returned
        //
        // if (mongoEntityExists(orionldState.payloadIdNode->value.s, orionldState.tenant) == true)
        // {
        //   orionldErrorResponseCreate(OrionldAlreadyExists, "Entity already exists", orionldState.payloadIdNode->value.s);
        //   ciP->httpStatusCode = SccConflict;
        //   return false;
        // }
        LM_TMP(("ID: %s", orionldState.payloadIdNode->value.s));
      }
      else if (strcmp(kNodeP->name, "type") == 0)
      {
        orionldState.payloadTypeNode = kNodeP;

        if (orionldState.payloadTypeNode->type != KjString)
        {
          orionldErrorResponseCreate(OrionldBadRequestData, "Invalid Entity Type", "Must be a JSON String");
          return false;
        }
      }
    }

    if (SCOMPARE9(kNodeP->name, 'l', 'o', 'c', 'a', 't', 'i', 'o', 'n', 0))
    {
      DUPLICATE_CHECK(locationNodeP, "location", kNodeP);
      // FIXME: check validity of location - GeoProperty - Issue #256
    }
    else if (SCOMPARE17(kNodeP->name, 'o', 'b', 's', 'e', 'r', 'v', 'a', 't', 'i', 'o', 'n', 'S', 'p', 'a', 'c', 'e', 0))
    {
      DUPLICATE_CHECK(observationSpaceNodeP, "observationSpace", kNodeP);
      // FIXME: check validity of observationSpace - GeoProperty
    }
    else if (SCOMPARE15(kNodeP->name, 'o', 'p', 'e', 'r', 'a', 't', 'i', 'o', 'n', 'S', 'p', 'a', 'c', 'e', 0))
    {
      DUPLICATE_CHECK(operationSpaceNodeP, "operationSpace", kNodeP);
      // FIXME: check validity of operationSpaceP - GeoProperty
    }
    else if (SCOMPARE10(kNodeP->name, 'c', 'r', 'e', 'a', 't', 'e', 'd', 'A', 't', 0))
    {
      DUPLICATE_CHECK(createdAtP, "createdAt", kNodeP);
      STRING_CHECK(kNodeP, "createdAt");
    }
    else if (SCOMPARE11(kNodeP->name, 'm', 'o', 'd', 'i', 'f', 'i', 'e', 'd', 'A', 't', 0))
    {
      DUPLICATE_CHECK(modifiedAtP, "modifiedAt", kNodeP);
      STRING_CHECK(kNodeP, "modifiedAt");
    }
    else  // Property/Relationshiop - must check chars in the name of the attribute
    {
      LM_TMP(("Name: %s | Value: %s", kNodeP->name, kNodeP->value.s));
      if (strcmp(kNodeP->name, "@context") != 0)
      {
        if (orionldValidName(kNodeP->name, &detailsP) == false)
        {
          orionldErrorResponseCreate(OrionldBadRequestData, "Invalid Property/Relationship name", detailsP);
          return false;
        }
      }
    }
    kNodeP = kNodeP->next;
  }

  if (isBatchOperation)
  {
    LM_TMP(("isBatchOperation TRUE"));
    if (checkEntityIdFieldExists() == false)
    {
      return false;
    }
  }


  //
  // Prepare output
  //
  *locationNodePP         = locationNodeP;
  *observationSpaceNodePP = observationSpaceNodeP;
  *operationSpaceNodePP   = operationSpaceNodeP;
  *createdAtPP            = createdAtP;
  *modifiedAtPP           = modifiedAtP;

  return true;
}
