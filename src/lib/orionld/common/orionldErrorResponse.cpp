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
* Author: Ken Zangelin
*/
extern "C"
{
#include "kjson/KjNode.h"                                      // KjNode
#include "kjson/kjBuilder.h"                                   // kjString
}

#include "logMsg/logMsg.h"                                     // LM_*
#include "logMsg/traceLevels.h"                                // Lmt*

#include "orionld/common/orionldState.h"                       // orionldState
#include "orionld/common/orionldErrorResponse.h"               // Own interface



// -----------------------------------------------------------------------------
//
// errorTypeStringV -
//
static const char* errorTypeStringV[] =
{
  "https://uri.etsi.org/ngsi-ld/errors/OK",
  "https://uri.etsi.org/ngsi-ld/errors/InvalidRequest",
  "https://uri.etsi.org/ngsi-ld/errors/BadRequestData",
  "https://uri.etsi.org/ngsi-ld/errors/AlreadyExists",
  "https://uri.etsi.org/ngsi-ld/errors/OperationNotSupported",
  "https://uri.etsi.org/ngsi-ld/errors/ResourceNotFound",
  "https://uri.etsi.org/ngsi-ld/errors/InternalError",
  "https://uri.etsi.org/ngsi-ld/errors/TooComplexQuery",
  "https://uri.etsi.org/ngsi-ld/errors/TooManyResults",
  "https://uri.etsi.org/ngsi-ld/errors/LdContextNotAvailable",
  "https://uri.etsi.org/ngsi-ld/errors/NoMultiTenantSupport",
  "https://uri.etsi.org/ngsi-ld/errors/NonExistingTenant"
};



// ----------------------------------------------------------------------------
//
// orionldErrorTypeToString -
//
const char* orionldErrorTypeToString(OrionldResponseErrorType type)
{
  return errorTypeStringV[type];
}



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
  const char*               detail
)
{
  LM_T(LmtErrorResponse, ("Creating error response: %s (%s)", title, detail));

  orionldState.pd.title  = (char*) title;
  orionldState.pd.detail = (char*) detail;

  if ((title  != NULL) && (detail != NULL))
  {
    snprintf(orionldState.pd.titleAndDetailBuffer, sizeof(orionldState.pd.titleAndDetailBuffer), "%s: %s", title, detail);
    orionldState.pd.titleAndDetail = orionldState.pd.titleAndDetailBuffer;
  }
  else if (title != NULL)
    orionldState.pd.titleAndDetail = (char*) title;
  else if (detail != NULL)
    orionldState.pd.titleAndDetail = (char*) detail;
  else
    orionldState.pd.titleAndDetail = (char*) "no error info available";

  KjNode* typeP     = kjString(orionldState.kjsonP, "type",    orionldErrorTypeToString(errorType));
  KjNode* titleP    = kjString(orionldState.kjsonP, "title",   title);
  KjNode* detailP;

  if ((detail != NULL) && (detail[0] != 0))
    detailP = kjString(orionldState.kjsonP, "detail", detail);
  else
    detailP = kjString(orionldState.kjsonP, "detail", "no detail");

  orionldState.responseTree = kjObject(orionldState.kjsonP, NULL);

  kjChildAdd(orionldState.responseTree, typeP);
  kjChildAdd(orionldState.responseTree, titleP);
  kjChildAdd(orionldState.responseTree, detailP);
}



// ----------------------------------------------------------------------------
//
// orionldErrorResponseCreate -
//
void orionldErrorResponseCreate(OrionldProblemDetails* pdP)
{
  orionldState.httpStatusCode = pdP->status;
  orionldState.pd             = *pdP;
  orionldErrorResponseCreate(pdP->type, pdP->title, pdP->detail);
}
