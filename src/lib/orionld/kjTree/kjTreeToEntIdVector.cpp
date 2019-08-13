/*
*
* Copyright 2019 Telefonica Investigacion y Desarrollo, S.A.U
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
#include <vector>

#include "logMsg/logMsg.h"                                     // LM_*
#include "logMsg/traceLevels.h"                                // Lmt*

extern "C"
{
#include "kjson/KjNode.h"                                      // KjNode
}

#include "rest/ConnectionInfo.h"                               // ConnectionInfo
#include "apiTypesV2/EntID.h"                                  // EntID
#include "orionld/common/SCOMPARE.h"                           // SCOMPAREx
#include "orionld/common/CHECK.h"                              // CHECKx(U)
#include "orionld/common/orionldState.h"                       // orionldState
#include "orionld/common/orionldErrorResponse.h"               // orionldErrorResponseCreate
#include "orionld/common/urlCheck.h"                           // urlCheck
#include "orionld/common/urnCheck.h"                           // urnCheck
#include "orionld/context/orionldUriExpand.h"                  // orionldUriExpand
#include "orionld/kjTree/kjTreeToEntIdVector.h"                // Own interface



// -----------------------------------------------------------------------------
//
// kjTreeToEntIdVector -
//
// ngsiv2::EntID is used, but the type is really EntityInfo, and it has three fields:
// - id
// - idPattern
// - type
//
bool kjTreeToEntIdVector(ConnectionInfo* ciP, KjNode* kNodeP, std::vector<ngsiv2::EntID>* entitiesP)
{
  KjNode* entityP;

  for (entityP = kNodeP->value.firstChildP; entityP != NULL; entityP = entityP->next)
  {
    if (entityP->type != KjObject)
    {
      orionldErrorResponseCreate(OrionldBadRequestData, "EntityInfo array member not a JSON Object", NULL, OrionldDetailsString);
      return false;
    }

    KjNode*  itemP;
    char*    idP        = NULL;
    char*    idPatternP = NULL;
    char*    typeP      = NULL;

    for (itemP = entityP->value.firstChildP; itemP != NULL; itemP = itemP->next)
    {
      if (SCOMPARE3(itemP->name, 'i', 'd', 0))
      {
        DUPLICATE_CHECK(idP, "EntityInfo::id", itemP->value.s);
        STRING_CHECK(itemP, "EntityInfo::id");
      }
      else if (SCOMPARE10(itemP->name, 'i', 'd', 'P', 'a', 't', 't', 'e', 'r', 'n', 0))
      {
        DUPLICATE_CHECK(idPatternP, "EntityInfo::idPattern", itemP->value.s);
        STRING_CHECK(itemP, "EntityInfo::idPattern");
      }
      else if (SCOMPARE5(itemP->name, 't', 'y', 'p', 'e', 0))
      {
        DUPLICATE_CHECK(typeP, "EntityInfo::type", itemP->value.s);
        STRING_CHECK(itemP, "EntityInfo::type");
      }
      else
      {
        orionldErrorResponseCreate(OrionldBadRequestData, "Unknown EntityInfo field", itemP->name, OrionldDetailsString);
        return false;
      }
    }

    if ((idP == NULL) && (idPatternP == NULL) && (typeP == NULL))
    {
      orionldErrorResponseCreate(OrionldBadRequestData, "Empty EntityInfo object", NULL, OrionldDetailsString);
      return false;
    }

    if ((idP != NULL) && (idPatternP != NULL))
    {
      orionldErrorResponseCreate(OrionldBadRequestData, "Both 'id' and 'idPattern' given in EntityInfo object", NULL, OrionldDetailsString);
      return false;
    }

    if (idP != NULL)
    {
      // The entity id must be a URI
      char* details;

      if ((urlCheck(idP, &details) == false) && (urnCheck(idP, &details) == false))
      {
        orionldErrorResponseCreate(OrionldBadRequestData, "Invalid Entity ID", details, OrionldDetailsString);
        ciP->httpStatusCode = SccBadRequest;
        return false;
      }
    }

    if (typeP == NULL)
    {
      orionldErrorResponseCreate(OrionldBadRequestData, "Missing field in EntityInfo object", "type", OrionldDetailsString);
      return false;
    }

    char  typeExpanded[256];
    char* details;

    if (orionldUriExpand(orionldState.contextP, typeP, typeExpanded, sizeof(typeExpanded), &details) == false)
    {
      orionldErrorResponseCreate(OrionldBadRequestData, "Error during URI expansion of entity type", details, OrionldDetailsString);
      return false;
    }
    typeP = typeExpanded;

    ngsiv2::EntID entityInfo;

    if (idP)        entityInfo.id        = idP;
    if (idPatternP) entityInfo.idPattern = idPatternP;
    if (typeP)      entityInfo.type      = typeP;

    entitiesP->push_back(entityInfo);
  }

  return true;
}
