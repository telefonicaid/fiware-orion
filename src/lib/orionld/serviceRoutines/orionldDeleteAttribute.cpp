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
// orionldDeleteAttribute -
//
bool orionldDeleteAttribute(ConnectionInfo* ciP)
{
  char*    entityId = orionldState.wildcard[0];
  char*    attrName = orionldState.wildcard[1];
  char*    attrNameP;
  char*    detail;

  // Make sure the Entity ID is a valid URI
  if (pcheckUri(entityId, &detail) == false)
  {
    LM_W(("Bad Input (Invalid Entity ID '%s' - not a URI)", entityId));
    orionldErrorResponseCreate(OrionldBadRequestData, "Invalid Entity ID", detail);  // FIXME: Include name (entityId) and value ($entityId)
    orionldState.httpStatusCode = SccBadRequest;
    return false;
  }

  if (dbEntityLookup(entityId) == NULL)
  {
    LM_T(LmtService, ("Entity Not Found: %s", entityId));
    orionldErrorResponseCreate(OrionldResourceNotFound, "The requested entity has not been found. Check its id", entityId);
    orionldState.httpStatusCode = SccNotFound;  // 404
    return false;
  }

  attrNameP = orionldAttributeExpand(orionldState.contextP, attrName, true, NULL);

  // IMPORTANT: Must call dbEntityAttributeLookup before replacing dots for eqs
  if (dbEntityAttributeLookup(entityId, attrNameP) == NULL)
  {
    LM_T(LmtService, ("Attribute Not Found: %s/%s", entityId, attrNameP));
    orionldState.httpStatusCode = SccContextElementNotFound;
    orionldErrorResponseCreate(OrionldBadRequestData, "Attribute Not Found", attrNameP);
    return false;
  }

  dotForEq(attrNameP);

  char* attrNameV[1]  = { attrNameP };
  if (dbEntityAttributesDelete(entityId, attrNameV, 1) == false)
  {
    LM_W(("dbEntityAttributesDelete failed"));
    orionldState.httpStatusCode = SccContextElementNotFound;
    orionldErrorResponseCreate(OrionldBadRequestData, "Attribute Not Found", attrNameP);
    return false;
  }

  orionldState.httpStatusCode = SccNoContent;
  return true;
}
