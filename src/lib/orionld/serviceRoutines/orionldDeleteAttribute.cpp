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
#include "kjson/kjClone.h"                                       // kjClone
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
    LM_TMP(("datasetId == NULL - Removing the entire dataset"));
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

  LM_TMP(("datasetId == '%s' - removing a single instance (To Be Implemented)", datasetId));
  //
  // Remove a single dataset instance
  //   1. Get the @datasets for 'attrNameExpandedEq':
  //      KjTree* datasetsP = dbDatasetGet(entityId, attrName, attrNameExpandedEq);
  //   2. Lookup the matching object (not found? 404)
  //   3. Remove the matching object from datasetsP
  //   4. Replace @datasets.attrNameExpandedEq with what's left in datasetsP
  //      mongoCppLegacyEntityFieldReplace(entityId, fieldPath, datasetsP);
#if 0
  KjNode* datasetsP = dbDatasetGet(entityId, attrName, attrNameExpandedEq);

  if (datasetsP == NULL)
  KjNode* toRemove  = 
#endif

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

  LM_TMP(("DA: Here"));
  //
  // URI param check: both datasetId and deleteAll cannot be set
  //
  if ((orionldState.uriParams.datasetId != NULL) && (orionldState.uriParams.deleteAll == true))
  {
    orionldState.httpStatusCode = 400;
    orionldErrorResponseCreate(OrionldBadRequestData, "Invalid URI param combination", "both datasetId and deleteAll are set");
    return false;
  }

  LM_TMP(("DA: Here"));
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

  LM_TMP(("DA: Here"));
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

  LM_TMP(("DA: Looking up E/A: '%s' / '%s'", entityId, attrNameExpanded));
  if (dbEntityAttributeLookup(entityId, attrNameExpanded) == NULL)
  {
    LM_TMP(("DA: Not Found"));
    orionldState.httpStatusCode = 404;
    orionldErrorResponseCreate(OrionldResourceNotFound, "Attribute not found", attrNameExpanded);
    return false;
  }

  char* attrNameExpandedEq = kaStrdup(&orionldState.kalloc, attrNameExpanded);
  dotForEq(attrNameExpandedEq);
  LM_TMP(("DA: attrName:           '%s'", attrName));
  LM_TMP(("DA: attrNameExpanded:   '%s'", attrNameExpanded));
  LM_TMP(("DA: attrNameExpandedEq: '%s'", attrNameExpandedEq));

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
  {
    LM_TMP(("DA: orionldState.uriParams.datasetId is set - calling orionldDeleteAttributeDatasetId(%s)", orionldState.uriParams.datasetId));
    return orionldDeleteAttributeDatasetId(entityId, attrNameExpanded, attrNameExpandedEq, orionldState.uriParams.datasetId);
  }
  else if (orionldState.uriParams.deleteAll == true)
  {
    LM_TMP(("DA: orionldState.uriParams.deleteAll is set - calling orionldDeleteAttributeDatasetId(NULL)"));
    if (orionldDeleteAttributeDatasetId(entityId, attrName, attrNameExpandedEq, NULL) == false)
      return false;
  }

  LM_TMP(("DA: Removing the default attribute"));

  char* attrNameV[1] = { attrNameExpandedEq };
  if (dbEntityAttributesDelete(entityId, attrNameV, 1) == false)
  {
    LM_W(("dbEntityAttributesDelete failed"));
    orionldState.httpStatusCode = 404;
    orionldErrorResponseCreate(OrionldResourceNotFound, "Attribute Not Found", attrNameExpandedEq);
    return false;
  }

  orionldState.httpStatusCode = SccNoContent;
  return true;
}
