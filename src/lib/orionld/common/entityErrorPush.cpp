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
* Author: Gabriel Quaresma and Ken Zangelin
*/
extern "C"
{
#include "kjson/KjNode.h"                                        // KjNode
#include "kjson/kjBuilder.h"                                     // kjString, kjObject, ...
#include "kjson/kjLookup.h"                                      // kjLookup
}

#include "logMsg/logMsg.h"                                       // LM_*
#include "logMsg/traceLevels.h"                                  // Lmt*

#include "orionld/common/orionldErrorResponse.h"                 // OrionldResponseErrorType
#include "orionld/common/orionldState.h"                         // orionldState



// -----------------------------------------------------------------------------
//
// entityErrorPush -
//
// The array "errors" in BatchOperationResult is an array of BatchEntityError.
// BatchEntityError contains a string (the entity id) and an instance of ProblemDetails.
//
// ProblemDetails is described in https://www.etsi.org/deliver/etsi_gs/CIM/001_099/009/01.01.01_60/gs_CIM009v010101p.pdf
// and contains:
//
// * type      (string) A URI reference that identifies the problem type
// * title     (string) A short, human-readable summary of the problem
// * detail    (string) A human-readable explanation specific to this occurrence of the problem
// * status    (number) The HTTP status code
// * instance  (string) A URI reference that identifies the specific occurrence of the problem
//
// Of these five items, only "type" seems to be mandatory.
//
// This implementation will treat "type", "title", and "status" as MANDATORY, and "detail" as OPTIONAL
//
void entityErrorPush(KjNode* errorsArrayP, const char* entityId, OrionldResponseErrorType type, const char* title, const char* detail, int status, bool avoidDuplicate)
{
  if (avoidDuplicate == true)
  {
    // If the entity 'entityId' is present already, then don't add another one
    for (KjNode* nodeP = errorsArrayP->value.firstChildP; nodeP != NULL; nodeP = nodeP->next)
    {
      KjNode* entityIdP = kjLookup(nodeP, "entityId");

      if ((entityIdP != NULL) && (strcmp(entityIdP->value.s, entityId) == 0))
        return;
    }
  }

  KjNode* objP            = kjObject(orionldState.kjsonP, NULL);
  KjNode* eIdP            = kjString(orionldState.kjsonP,  "entityId", entityId);
  KjNode* problemDetailsP = kjObject(orionldState.kjsonP,  "error");
  KjNode* typeP           = kjString(orionldState.kjsonP,  "type",     orionldErrorTypeToString(type));
  KjNode* titleP          = kjString(orionldState.kjsonP,  "title",    title);
  KjNode* statusP         = kjInteger(orionldState.kjsonP, "status",   status);

  kjChildAdd(problemDetailsP, typeP);
  kjChildAdd(problemDetailsP, titleP);

  if (detail != NULL)
  {
    KjNode* detailP = kjString(orionldState.kjsonP, "detail", detail);
    kjChildAdd(problemDetailsP, detailP);
  }

  kjChildAdd(problemDetailsP, statusP);

  kjChildAdd(objP, eIdP);
  kjChildAdd(objP, problemDetailsP);

  kjChildAdd(errorsArrayP, objP);
}
