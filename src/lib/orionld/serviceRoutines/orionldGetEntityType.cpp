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
* Author: Ken Zangelin
*/
extern "C"
{
#include "kjson/KjNode.h"                                        // KjNode
}

#include "logMsg/logMsg.h"                                       // LM_*
#include "logMsg/traceLevels.h"                                  // Lmt*

#include "rest/ConnectionInfo.h"                                 // ConnectionInfo

#include "orionld/common/orionldState.h"                         // orionldState
#include "orionld/common/orionldErrorResponse.h"                 // orionldErrorResponseCreate
#include "orionld/types/OrionldProblemDetails.h"                 // OrionldProblemDetails
#include "orionld/rest/OrionLdRestService.h"                     // OrionLdRestService
#include "orionld/db/dbEntityTypeGet.h"                          // dbEntityTypeGet
#include "orionld/serviceRoutines/orionldGetEntityType.h"        // Own Interface



// ----------------------------------------------------------------------------
//
// orionldGetEntityType -
//
bool orionldGetEntityType(ConnectionInfo* ciP)
{
#if 1
  orionldState.httpStatusCode = 501;
  orionldState.noLinkHeader   = true;  // We don't want the Link header for non-implemented requests

  orionldErrorResponseCreate(OrionldOperationNotSupported, "Not Implemented", orionldState.serviceP->url);
  return false;
#else
  OrionldProblemDetails  pd;
  char*                  type = orionldContextItemExpand(orionldState.contextP, orionldState.wildcard[0], true, NULL);

  orionldState.responseTree = dbEntityTypeGet(&pd, type);
  if (orionldState.responseTree == NULL)
  {
    LM_E(("dbEntityTypeGet: %s: %s", pd.title, pd.detail));
    orionldErrorResponseCreate(OrionldResourceNotFound, pd.title, pd.detail);
    orionldState.httpStatusCode = (HttpStatusCode) pd.status;
    return false;
  }

  return true;
#endif
}
