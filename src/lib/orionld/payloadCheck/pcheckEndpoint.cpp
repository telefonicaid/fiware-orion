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
#include "kjson/KjNode.h"                                       // KjNode
}

#include "logMsg/logMsg.h"                                      // LM_*
#include "logMsg/traceLevels.h"                                 // Lmt*

#include "orionld/common/CHECK.h"                               // STRING_CHECK, ...
#include "orionld/common/orionldState.h"                        // orionldState
#include "orionld/payloadCheck/pcheckEndpoint.h"                // Own interface



// ----------------------------------------------------------------------------
//
// pcheckEndpoint -
//
bool pcheckEndpoint(KjNode* endpointP)
{
  KjNode* uriP    = NULL;
  KjNode* acceptP = NULL;

  for (KjNode* epItemP = endpointP->value.firstChildP; epItemP != NULL; epItemP = epItemP->next)
  {
    if (strcmp(epItemP->name, "uri") == 0)
    {
      DUPLICATE_CHECK(uriP, "endpoint::uri", epItemP);
      STRING_CHECK(uriP, "endpoint::uri");
      URI_CHECK(uriP->value.s, "endpoint::uri", true);
    }
    else if (strcmp(epItemP->name, "accept") == 0)
    {
      DUPLICATE_CHECK(acceptP, "endpoint::accept", epItemP);
      STRING_CHECK(acceptP, "endpoint::accept");
      EMPTY_STRING_CHECK(acceptP, "endpoint::accept");
      if ((strcmp(acceptP->value.s, "application/json") != 0) && (strcmp(acceptP->value.s, "application/ld+json") != 0))
      {
        orionldErrorResponseCreate(OrionldBadRequestData, "Unsupported Mime-type in 'accept'", epItemP->value.s);
        orionldState.httpStatusCode = SccBadRequest;
        return false;
      }
    }
    else
    {
      orionldErrorResponseCreate(OrionldBadRequestData, "Invalid field for 'endpoint'", epItemP->name);
      orionldState.httpStatusCode = SccBadRequest;
      return false;
    }
  }

  if (uriP == NULL)
  {
    orionldErrorResponseCreate(OrionldBadRequestData, "Mandatory field missing in 'endpoint'", "uri");
    orionldState.httpStatusCode = SccBadRequest;
    return false;
  }

  return true;
}
