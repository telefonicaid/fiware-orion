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
extern "C"
{
#include "kjson/KjNode.h"                                      // KjNode
#include "kjson/kjBuilder.h"                                   // kjString
}

#include "logMsg/logMsg.h"                                     // LM_*
#include "logMsg/traceLevels.h"                                // Lmt*

#include "orionld/context/orionldCoreContext.h"                // orionldDefaultUrl
#include "orionld/context/orionldContextItemLookup.h"          // orionldContextItemLookup
#include "orionld/common/orionldState.h"                       // orionldState
#include "orionld/common/orionldErrorResponse.h"               // Own interface



// -----------------------------------------------------------------------------
//
// errorTypeStringV -
//
static const char* errorTypeStringV[] =
{
  "https://uri.etsi.org/ngsi-ld/errors/InvalidRequest",
  "https://uri.etsi.org/ngsi-ld/errors/BadRequestData",
  "https://uri.etsi.org/ngsi-ld/errors/AlreadyExists",
  "https://uri.etsi.org/ngsi-ld/errors/OperationNotSupported",
  "https://uri.etsi.org/ngsi-ld/errors/ResourceNotFound",
  "https://uri.etsi.org/ngsi-ld/errors/InternalError",
  "https://uri.etsi.org/ngsi-ld/errors/OrionldTooComplexQuery",
  "https://uri.etsi.org/ngsi-ld/errors/OrionldTooManyResults",
  "https://uri.etsi.org/ngsi-ld/errors/OrionldLdContextNotAvailable"
};



// ----------------------------------------------------------------------------
//
// orionldErrorResponseCreate -
//
// NOTE
//   Only service routines should use this function.
//   Lower level functions should just return the 'detail' string.
//
void orionldErrorResponseCreate
(
  OrionldResponseErrorType  errorType,
  const char*               title,
  const char*               detail,
  OrionldDetailType         detailType
)
{
  LM_T(LmtErrorResponse, ("Creating error response: %s (%s)", title, detail));

  KjNode* typeP     = kjString(orionldState.kjsonP, "type",    errorTypeStringV[errorType]);
  KjNode* titleP    = kjString(orionldState.kjsonP, "title",   title);
  KjNode* detailP;

  if ((detail != NULL) && (detail[0] != 0))
  {
    char*   contextDetail = NULL;

    if (detailType == OrionldDetailString)  // no replacement as it's just a descriptive string
    {
      contextDetail = (char*) detail;
    }
    else  // lookup 'detail' in context
    {
      KjNode*  nodeP = orionldContextItemLookup(orionldState.contextP, detail);
      char     contextDetailV[512];  // FIXME: Define a max length for a context item?

      if (nodeP == NULL)
      {
        snprintf(contextDetailV, sizeof(contextDetailV), "%s%s", orionldDefaultUrl, detail);
        contextDetail = contextDetailV;
      }
      else
      {
        contextDetail = nodeP->value.s;
      }
    }

    detailP = kjString(orionldState.kjsonP, "detail", contextDetail);
  }
  else
    detailP = kjString(orionldState.kjsonP, "detail", "no detail");

  orionldState.responseTree = kjObject(orionldState.kjsonP, NULL);

  kjChildAdd(orionldState.responseTree, typeP);
  kjChildAdd(orionldState.responseTree, titleP);
  kjChildAdd(orionldState.responseTree, detailP);
}
