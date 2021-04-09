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
* Author: Ken Zangelin and Gabriel Quaresma
*/
#include <string>                                                // std::string  - for servicePath only
#include <vector>                                                // std::vector  - for servicePath only

extern "C"
{
#include "kbase/kMacros.h"                                       // K_VEC_SIZE, K_FT
#include "kjson/kjBuilder.h"                                     // kjChildRemove
#include "kjson/kjLookup.h"                                      // kjLookup
#include "kalloc/kaAlloc.h"                                      // kaAlloc
#include "kalloc/kaStrdup.h"                                     // kaStrdup
}

#include "logMsg/logMsg.h"                                       // LM_*
#include "logMsg/traceLevels.h"                                  // Lmt*

#include "rest/ConnectionInfo.h"                                 // ConnectionInfo
#include "rest/HttpStatusCode.h"                                 // SccContextElementNotFound

#include "orionld/common/orionldErrorResponse.h"                 // orionldErrorResponseCreate
#include "orionld/common/httpStatusCodeToOrionldErrorType.h"     // httpStatusCodeToOrionldErrorType
#include "orionld/common/orionldState.h"                         // orionldState
#include "orionld/common/dotForEq.h"                             // dotForEq
#include "orionld/common/eqForDot.h"                             // eqForDot
#include "orionld/payloadCheck/pcheckUri.h"                      // pcheckUri
#include "orionld/db/dbConfiguration.h"                          // dbEntityAttributeLookup, dbEntityAttributesDelete
#include "orionld/context/orionldAttributeExpand.h"              // orionldAttributeExpand
#include "orionld/serviceRoutines/orionldDeleteAttribute.h"      // Own Interface



// ----------------------------------------------------------------------------
//
// orionldDeleteAttributeDatasetId -
//
// PARAMETERS
//   entityId             The ID of the entity
//   attrNameExpanded     Expanded Attribute Name
//   attrNameExpandedEq   Expanded Attribute Name with dots already replaced for EQ signs
//   datasetId            If NULL - all datasets for the attribute are deleted, else only the matching one
//
bool orionldDeleteAttributeDatasetId(const char* entityId, const char* attrNameExpanded, const char* attrNameExpandedEq, const char* datasetId)
{
  char fieldPath[512];

  //
  // Remove the entire dataset?
  //
  if (datasetId == NULL)
  {
    snprintf(fieldPath, sizeof(fieldPath), "@datasets.%s", attrNameExpandedEq);
    if (dbEntityFieldDelete(entityId, fieldPath) == false)
    {
      // orionldState.httpStatusCode = 500;
      // orionldErrorResponseCreate(OrionldInternalError, "Database Error (unable to remove an entire dataset)", fieldPath);
      orionldState.httpStatusCode = 404;
      orionldErrorResponseCreate(OrionldResourceNotFound, "Attribute datasets not found", attrNameExpanded);
      return false;
    }

    orionldState.httpStatusCode = SccNoContent;

    return true;
  }


  //
  // Remove a single dataset instance
  //   1. Get the @datasets for 'attrNameExpandedEq':
  //      KjTree* datasetsP = dbDatasetGet(entityId, attrName, attrNameExpandedEq);
  //   2. Lookup the matching object (not found? 404)
  //   3. Remove the matching object from datasetsP
  //   4. Replace @datasets.attrNameExpandedEq with what's left in datasetsP
  //      mongoCppLegacyEntityFieldReplace(entityId, fieldPath, datasetsP);
  KjNode* datasetsP = dbDatasetGet(entityId, attrNameExpandedEq, datasetId);

  if (datasetsP == NULL)
  {
    orionldState.httpStatusCode = 404;
    orionldErrorResponseCreate(OrionldResourceNotFound, "Attribute datasets not found", attrNameExpanded);
    return false;
  }

  char datasetPath[512];
  snprintf(datasetPath, sizeof(datasetPath), "@datasets.%s", attrNameExpandedEq);

  if (datasetsP->type == KjArray)
  {
    bool found = false;

    for (KjNode* instanceP = datasetsP->value.firstChildP; instanceP != NULL; instanceP = instanceP->next)
    {
      KjNode* datasetIdNode = kjLookup(instanceP, "datasetId");

      if ((datasetIdNode != NULL) && (strcmp(datasetIdNode->value.s, datasetId) == 0))
      {
        // Found it
        kjChildRemove(datasetsP, instanceP);
        found = true;
        break;
      }
    }

    if (found == false)
    {
      orionldState.httpStatusCode = 404;
      orionldErrorResponseCreate(OrionldResourceNotFound, "Attribute dataset not found", datasetId);
      return false;
    }

    dbEntityFieldReplace(entityId, datasetPath, datasetsP);
  }
  else
  {
    //
    // One single instance - if it matches, the entire item in @datasets is removed
    //                                      else: 404
    //
    KjNode* datasetIdNode = kjLookup(datasetsP, "datasetId");

    if ((datasetIdNode == NULL) || (strcmp(datasetIdNode->value.s, datasetId) != 0))
    {
      orionldState.httpStatusCode = 404;
      orionldErrorResponseCreate(OrionldResourceNotFound, "Attribute dataset not found", datasetId);
      return false;
    }

    // It matched - the entire item in @datasets is removed
    dbEntityFieldDelete(entityId, datasetPath);
  }

  orionldState.httpStatusCode = SccNoContent;
  return true;
}


// ----------------------------------------------------------------------------
//
// orionldDeleteAttribute -
//
bool orionldDeleteAttribute(ConnectionInfo* ciP)
{
  char*    entityId = orionldState.wildcard[0];
  char*    attrName = orionldState.wildcard[1];
  char*    attrNameExpanded;
  char*    detail;

  //
  // URI param check: both datasetId and deleteAll cannot be set
  //
  if ((orionldState.uriParams.datasetId != NULL) && (orionldState.uriParams.deleteAll == true))
  {
    orionldState.httpStatusCode = 400;
    orionldErrorResponseCreate(OrionldBadRequestData, "Invalid URI param combination", "both datasetId and deleteAll are set");
    return false;
  }

  //
  // Make sure the Entity ID is a valid URI
  //
  if (pcheckUri(entityId, true, &detail) == false)
  {
    LM_W(("Bad Input (Invalid Entity ID '%s' - not a URI)", entityId));
    orionldErrorResponseCreate(OrionldBadRequestData, "Invalid Entity ID", detail);  // FIXME: Include name (entityId) and value ($entityId)
    orionldState.httpStatusCode = SccBadRequest;
    return false;
  }

  //
  // Make sure the Attribute Name is valid
  //
  if (pcheckUri(attrName, false, &detail) == false)
  {
    LM_W(("Bad Input (Invalid Attribute Name (%s): %s)", attrName, detail));
    orionldErrorResponseCreate(OrionldBadRequestData, "Invalid Attribute Name", detail);  // FIXME: Include name (entityId) and value ($entityId)
    orionldState.httpStatusCode = SccBadRequest;
    return false;
  }

  attrNameExpanded = orionldAttributeExpand(orionldState.contextP, attrName, true, NULL);

  if (dbEntityAttributeLookup(entityId, attrNameExpanded) == NULL)
  {
    orionldState.httpStatusCode = 404;
    orionldErrorResponseCreate(OrionldResourceNotFound, "Entity/Attribute not found", attrNameExpanded);
    return false;
  }

  char* attrNameExpandedEq = kaStrdup(&orionldState.kalloc, attrNameExpanded);
  dotForEq(attrNameExpandedEq);

  //
  // Three possibilities here (well, four, if we count the error of "both SET"):
  //
  // URI Param datasetId   URI Param deleteAll  Action
  // -------------------   -------------------  ---------------------
  //      SET                    SET            Error - already taken care of
  //      SET                    NOT SET        Delete only the dataset instance with the matching datasetId
  //      NOT SET                SET            Delete both the default attribute and ALL dataset instances
  //      NOT SET                NOT SET        Delete only the default attribute
  //
  if (orionldState.uriParams.datasetId != NULL)
    return orionldDeleteAttributeDatasetId(entityId, attrNameExpanded, attrNameExpandedEq, orionldState.uriParams.datasetId);
  else if (orionldState.uriParams.deleteAll == true)
  {
    if (orionldDeleteAttributeDatasetId(entityId, attrName, attrNameExpandedEq, NULL) == false)
      return false;
  }

  char* attrNameV[1] = { attrNameExpandedEq };
  if (dbEntityAttributesDelete(entityId, attrNameV, 1) == false)
  {
    LM_W(("dbEntityAttributesDelete failed"));
    orionldState.httpStatusCode = 404;
    orionldErrorResponseCreate(OrionldResourceNotFound, "Entity/Attribute Not Found", attrNameExpanded);
    return false;
  }

  orionldState.httpStatusCode = SccNoContent;
  return true;
}
