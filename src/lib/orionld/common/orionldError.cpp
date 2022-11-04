/*
*
* Copyright 2022 FIWARE Foundation e.V.
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
#include "kalloc/kaStrdup.h"                                  // kaStrdup
}

#include "logMsg/logMsg.h"                                    // LM_*

#include "orionld/types/OrionldResponseErrorType.h"           // OrionldResponseErrorType
#include "orionld/types/OrionldProblemDetails.h"              // OrionldProblemDetails
#include "orionld/common/orionldState.h"                      // orionldState
#include "orionld/common/orionldError.h"                      // Own interface



// ----------------------------------------------------------------------------
//
// orionldError -
//
// ToDo:
// * Make the function accept also filename and line number
// * Use a macro for calling the function:
//   #define ERR(typ, title, detail, status)
//     orionldError(typ, title, detail, status, __FILE__, __LINE__
// * Instead of overwritinf every error, stack them. Keep all of them
//
void orionldError
(
  OrionldResponseErrorType  errorType,
  const char*               title,
  const char*               detail,
  int                       status
)
{
  orionldState.pd.type    = errorType;
  orionldState.pd.title   = (title  != NULL)? kaStrdup(&orionldState.kalloc, title)  : NULL;
  orionldState.pd.detail  = (detail != NULL)? kaStrdup(&orionldState.kalloc, detail) : NULL;
  orionldState.pd.status  = status;

  orionldState.httpStatusCode = status;  // FIXME: To Remove - Use orionldState.pd.status instead?

  LM_E(("***** ERROR %s: %s (status code: %d)", title, detail, status));
}



// ----------------------------------------------------------------------------
//
// orionldError2 - to be used together with the macros OERROR/OERRORV
//
void orionldError2
(
  OrionldResponseErrorType  errorType,
  const char*               title,
  const char*               detail,
  int                       status,
  const char*               fileName,
  int                       lineNo,
  const char*               funcName
)
{
  orionldState.pd.type    = errorType;
  orionldState.pd.title   = (title  != NULL)? kaStrdup(&orionldState.kalloc, title)  : NULL;
  orionldState.pd.detail  = (detail != NULL)? kaStrdup(&orionldState.kalloc, detail) : NULL;
  orionldState.pd.status  = status;
  // Copy also fileName, lineNo, funcName into orionldState.pd?

  orionldState.httpStatusCode = status;  // FIXME: To Remove - Use orionldState.pd.status instead

  LM_E(("***** ERROR %s: %s | sc: %d | %s[%d]:%s", title, detail, status, fileName, lineNo, funcName));
}
