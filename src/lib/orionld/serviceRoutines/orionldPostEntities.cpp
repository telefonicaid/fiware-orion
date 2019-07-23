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
#include "orionld/context/orionldUriExpand.h"                    // orionldUriExpand
#include "orionld/serviceRoutines/orionldPostEntities.h"         // Own interface



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

    *detailsPP = (char*) "invalid character in name";
    return false;
  }

  return true;
}



// -----------------------------------------------------------------------------
//
// payloadCheck -
//
static bool payloadCheck
(
  ConnectionInfo*  ciP,
  KjNode**         locationNodePP,
  KjNode**         observationSpaceNodePP,
  KjNode**         operationSpaceNodePP,
  KjNode**         createdAtPP,
  KjNode**         modifiedAtPP
)
{
  OBJECT_CHECK(orionldState.requestTree, "toplevel");

  KjNode*  kNodeP                 = orionldState.requestTree->value.firstChildP;
  KjNode*  locationNodeP          = NULL;
  KjNode*  observationSpaceNodeP  = NULL;
  KjNode*  operationSpaceNodeP    = NULL;
  KjNode*  createdAtP             = NULL;
  KjNode*  modifiedAtP            = NULL;

  //
  // Check presence of mandatory fields
  //
  if (orionldState.payloadIdNode == NULL)
  {
    orionldErrorResponseCreate(ciP, OrionldBadRequestData, "Entity id is missing", "The 'id' field is mandatory", OrionldDetailsString);
    return false;
  }

  if (orionldState.payloadTypeNode == NULL)
  {
    orionldErrorResponseCreate(ciP, OrionldBadRequestData, "Entity type is missing", "The type field is mandatory", OrionldDetailsString);
    return false;
  }


  //
  // Check for duplicated items and that data types are correct
  //
  while (kNodeP != NULL)
  {
    char* detailsP;

    if (SCOMPARE9(kNodeP->name, 'l', 'o', 'c', 'a', 't', 'i', 'o', 'n', 0))
    {
      DUPLICATE_CHECK(locationNodeP, "location", kNodeP);
      // FIXME: check validity of location - GeoProperty
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
      // FIXME: Make sure the type is either Property or Relationship
      if (orionldValidName(kNodeP->name, &detailsP) == false)
      {
        orionldErrorResponseCreate(ciP, OrionldBadRequestData, "Invalid Property/Relationship name", detailsP, OrionldDetailsString);
        return false;
      }
    }

    kNodeP = kNodeP->next;
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



// ----------------------------------------------------------------------------
//
// orionldPostEntities -
//
bool orionldPostEntities(ConnectionInfo* ciP)
{
  OBJECT_CHECK(orionldState.requestTree, "toplevel");

  char*    details;
  KjNode*  locationP          = NULL;
  KjNode*  observationSpaceP  = NULL;
  KjNode*  operationSpaceP    = NULL;
  KjNode*  createdAtP         = NULL;
  KjNode*  modifiedAtP        = NULL;

  if (payloadCheck(ciP, &locationP, &observationSpaceP, &operationSpaceP, &createdAtP, &modifiedAtP) == false)
    return false;

  LM_TMP(("In orionldPostEntities. orionldState.payloadIdNode at %p", orionldState.payloadIdNode));
  LM_TMP(("In orionldPostEntities. orionldState.payloadTypeNode at %p", orionldState.payloadTypeNode));

  char*    entityId           = orionldState.payloadIdNode->value.s;
  char*    entityType         = orionldState.payloadTypeNode->value.s;

  if ((urlCheck(entityId, &details) == false) && (urnCheck(entityId, &details) == false))
  {
    orionldErrorResponseCreate(ciP, OrionldBadRequestData, "Invalid Entity id", "The id specified cannot be resolved to a URL or URN", OrionldDetailsString);
    return false;
  }


  //
  // If the entity already exists, an error should be returned
  //
  LM_TMP(("In orionldPostEntities"));
  if (mongoEntityExists(entityId, orionldState.tenant) == true)
  {
    orionldErrorResponseCreate(ciP, OrionldAlreadyExists, "Entity already exists", entityId, OrionldDetailsString);
    ciP->httpStatusCode = SccConflict;
    return false;
  }

  LM_TMP(("In orionldPostEntities"));
  orionldState.entityId = entityId;

  LM_TMP(("In orionldPostEntities"));
  UpdateContextRequest   mongoRequest;
  UpdateContextResponse  mongoResponse;
  ContextElement*        ceP       = new ContextElement();  // FIXME: Any way I can avoid to allocate ?
  EntityId*              entityIdP;

  LM_TMP(("In orionldPostEntities"));
  mongoRequest.contextElementVector.push_back(ceP);
  entityIdP = &mongoRequest.contextElementVector[0]->entityId;
  mongoRequest.updateActionType = ActionTypeAppend;
  LM_TMP(("In orionldPostEntities"));

  entityIdP->id        = entityId;
  entityIdP->type      = (orionldState.payloadTypeNode != NULL)? orionldState.payloadTypeNode->value.s : NULL;
  entityIdP->isPattern = "false";
  entityIdP->creDate   = getCurrentTime();
  entityIdP->modDate   = getCurrentTime();
  LM_TMP(("In orionldPostEntities"));

  //
  // Entity TYPE
  //
  entityIdP->isTypePattern = false;

  LM_T(LmtUriExpansion, ("Looking up uri expansion for the entity type '%s'", entityType));
  LM_T(LmtUriExpansion, ("------------- uriExpansion for Entity Type starts here ------------------------------"));

  char*  expandedName;
  char*  expandedType;

  // FIXME: Call orionldUriExpand() - this here is a "copy" of what orionldUriExpand does
  extern int uriExpansion(OrionldContext* contextP, const char* name, char** expandedNameP, char** expandedTypeP, char** detailsPP);
  int    expansions = uriExpansion(orionldState.contextP, entityType, &expandedName, &expandedType, &details);
  LM_T(LmtUriExpansion, ("URI Expansion for type '%s': '%s'", entityType, expandedName));
  LM_T(LmtUriExpansion, ("Got %d expansions", expansions));

  if (expansions == 0)
  {
    // Expansion found in Core Context - perform no expansion
  }
  else if (expansions == 1)
  {
    // Take the long name, just ... NOT expandedType but expandedName. All good
    entityIdP->type = expandedName;
  }
  else if (expansions == -2)
  {
    // No expansion found in Core Context, and in no other contexts either - use default URL
    LM_T(LmtUriExpansion, ("EXPANSION: use default URL for entity type '%s'", entityType));
    entityIdP->type = orionldDefaultUrl;
    entityIdP->type += entityType;
  }
  else if (expansions == -1)
  {
    orionldErrorResponseCreate(ciP, OrionldBadRequestData, "Invalid context item for 'entity type'", details, OrionldDetailsString);
    mongoRequest.release();
    return false;
  }
  else  // expansions == 2 ... may be an incorrect context
  {
    orionldErrorResponseCreate(ciP, OrionldBadRequestData, "Invalid value of context item 'entity type'", orionldState.contextP->url, OrionldDetailsString);
    mongoRequest.release();
    return false;
  }



  // Treat the entire payload
  for (KjNode* kNodeP = orionldState.requestTree->value.firstChildP; kNodeP != NULL; kNodeP = kNodeP->next)
  {
    LM_T(LmtUriExpansion, ("treating entity node '%s'", kNodeP->name));

    if (kNodeP == createdAtP)
    {
      // FIXME: Make sure the value is a valid timestamp?

      // Ignore the 'createdAt' - See issue #43 (orionld)
#if 0
      // 2. Save it for future use (when creating the entity)
      if ((orionldState.overriddenCreationDate = parse8601Time(createdAtP->value.s)) == -1)
      {
        orionldErrorResponseCreate(ciP, OrionldBadRequestData, "Invalid value for 'createdAt' attribute", createdAtP->value.s, OrionldDetailsString);
        mongoRequest.release();
        return false;
      }
#endif
    }
    else if (kNodeP == modifiedAtP)
    {
      // FIXME: Make sure the value is a valid timestamp?

      // Ignore the 'createdAt' - See issue #43 (orionld)
#if 0
      // 2. Save it for future use (when creating the entity)
      if ((orionldState.overriddenModificationDate = parse8601Time(modifiedAtP->value.s)) == -1)
      {
        orionldErrorResponseCreate(ciP, OrionldBadRequestData, "Invalid value for 'modifiedAt' attribute", modifiedAtP->value.s, OrionldDetailsString);
        mongoRequest.release();
        return false;
      }
#endif
    }
    else  // Must be an attribute
    {
      LM_T(LmtPayloadCheck, ("Not createdAt/modifiedAt: '%s' - treating as ordinary attribute", kNodeP->name));

      ContextAttribute* caP            = new ContextAttribute();
      KjNode*           attrTypeNodeP  = NULL;

      if (orionldAttributeTreat(ciP, kNodeP, caP, &attrTypeNodeP) == false)
      {
        LM_E(("orionldAttributeTreat failed"));
        delete caP;
        mongoRequest.release();
        return false;
      }


      //
      // URI Expansion for Attribute NAME - except if its name is one of the following three:
      // 1. location
      // 2. observationSpace
      // 3. operationSpace
      //
      if ((strcmp(kNodeP->name, "location") == 0) || (strcmp(kNodeP->name, "observationSpace") == 0) || (strcmp(kNodeP->name, "operationSpace") == 0))
      {
        caP->name = kNodeP->name;
      }
      else
      {
        char longName[256];

        if (orionldUriExpand(orionldState.contextP, kNodeP->name, longName, sizeof(longName), &details) == false)
        {
          LM_E(("orionldUriExpand failed"));
          delete caP;
          mongoRequest.release();
          orionldErrorResponseCreate(ciP, OrionldBadRequestData, details, kNodeP->name, OrionldDetailsAttribute);
          return false;
        }

        caP->name = longName;
      }


      //
      // NO URI Expansion for Attribute TYPE
      //
      caP->type = attrTypeNodeP->value.s;

      ceP->contextAttributeVector.push_back(caP);
    }
  }

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

  mongoRequest.release();
  mongoResponse.release();

  if (ciP->httpStatusCode != SccOk)
  {
    LM_E(("mongoUpdateContext: HTTP Status Code: %d", ciP->httpStatusCode));
    orionldErrorResponseCreate(ciP, OrionldBadRequestData, "Internal Error", "Error from Mongo-DB backend", OrionldDetailsString);
    return false;
  }

  ciP->httpStatusCode = SccCreated;
  orionldState.entityCreated = true;

  httpHeaderLocationAdd(ciP, "/ngsi-ld/v1/entities/", entityId);

  return true;
}
