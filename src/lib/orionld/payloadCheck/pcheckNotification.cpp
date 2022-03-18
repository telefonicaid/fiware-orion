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
#include "orionld/common/orionldErrorResponse.h"                // orionldErrorResponseCreate
#include "orionld/context/orionldAttributeExpand.h"             // orionldAttributeExpand
#include "orionld/payloadCheck/pcheckEndpoint.h"                // pcheckEndpoint
#include "orionld/payloadCheck/pcheckNotification.h"            // Own interface



// -----------------------------------------------------------------------------
//
// pcheckNotification -
//
bool pcheckNotification(KjNode* notificationP)
{
  KjNode* attributesP = NULL;
  KjNode* formatP     = NULL;
  KjNode* endpointP   = NULL;

  OBJECT_CHECK(notificationP, "notification");
  EMPTY_OBJECT_CHECK(notificationP, "notification");

  for (KjNode* nItemP = notificationP->value.firstChildP; nItemP != NULL; nItemP = nItemP->next)
  {
    if (strcmp(nItemP->name, "attributes") == 0)
    {
      DUPLICATE_CHECK(attributesP, "attributes", nItemP);
      ARRAY_CHECK(nItemP, "attributes");
      EMPTY_ARRAY_CHECK(nItemP, "attributes");

      for (KjNode* attrP = nItemP->value.firstChildP; attrP != NULL; attrP = attrP->next)
      {
        STRING_CHECK(attrP, "attributes array item");
        attrP->value.s = orionldAttributeExpand(orionldState.contextP, attrP->value.s, true, NULL);
      }
    }
    else if (strcmp(nItemP->name, "format") == 0)
    {
      DUPLICATE_CHECK(formatP, "format", nItemP);
      STRING_CHECK(formatP, "format");
      EMPTY_STRING_CHECK(formatP, "format");
      if ((strcmp(formatP->value.s, "keyValues") != 0) && (strcmp(formatP->value.s, "normalized") != 0))
      {
        orionldErrorResponseCreate(OrionldBadRequestData, "Invalid value of 'format' (must be either 'keyValues' or 'normalized'", formatP->value.s);
        orionldState.httpStatusCode = SccBadRequest;
        return false;
      }
    }
    else if (strcmp(nItemP->name, "endpoint") == 0)
    {
      DUPLICATE_CHECK(endpointP, "endpoint", nItemP);
      OBJECT_CHECK(endpointP, "endpoint");
      EMPTY_OBJECT_CHECK(endpointP, "endpoint");
      if (pcheckEndpoint(endpointP) == false)
        return false;
    }
    else if (strcmp(nItemP->name, "status") == 0)
    {
      orionldErrorResponseCreate(OrionldBadRequestData, "Invalid field for notification", "'status' is read-only");
      orionldState.httpStatusCode = SccBadRequest;
      return false;
    }
    else
    {
      orionldErrorResponseCreate(OrionldBadRequestData, "Invalid field for notification", nItemP->name);
      orionldState.httpStatusCode = SccBadRequest;
      return false;
    }
  }

  if (endpointP == NULL)
  {
    orionldErrorResponseCreate(OrionldBadRequestData, "Mandatory field missing", "endpoint");
    orionldState.httpStatusCode = SccBadRequest;
    return false;
  }

  return true;
}
