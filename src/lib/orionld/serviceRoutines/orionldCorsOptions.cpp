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
#include "logMsg/logMsg.h"                                 // LM_*

#include "orionld/common/orionldState.h"                   // orionldState
#include "orionld/common/orionldError.h"                   // orionldError
#include "orionld/types/OrionldHeader.h"                   // orionldHeaderAdd
#include "orionld/rest/orionldServiceLookup.h"             // orionldServiceLookup
#include "orionld/rest/orionldServiceInit.h"               // orionldRestServiceV
#include "orionld/serviceRoutines/orionldCorsOptions.h"    // Own interface



// -----------------------------------------------------------------------------
//
// orionldAllowedVerbs - get a list of allowed verbs for a URL PATH
//
// FIXME: orionldBadVerb does the exact same thing. This functions should have its own module.
//
static char* orionldAllowedVerbs(char* verbList, int len)
{
  uint16_t  bitmask = 0;

  //
  // There are nine verbs/methods, but only GET, POST, PATCH and DELETE are supported by ORIONLD
  // This loop looks up the URL PATH for each "orionld-valid" verb and keeps a bitmask of the hits
  //
  for (uint16_t verbNo = 0; verbNo <= PATCH; verbNo++)  // 0:GET, 1:PUT, 2:POST, 3:DELETE, 4:PATCH
  {
    if (verbNo == PUT) continue;

    if (orionldServiceLookup(&orionldRestServiceV[verbNo]) != NULL)
    {
      bitmask |= (1 << verbNo);
    }
  }

  verbList[0] = 0;

  if (bitmask > 0)
  {
    bool corsEnabled = allowedOrigin[0] != 0;

    if  (bitmask & (1 << GET))                     strcat(verbList, ",GET");
    if  (bitmask & (1 << POST))                    strcat(verbList, ",POST");
    if  (bitmask & (1 << DELETE))                  strcat(verbList, ",PATCH");
    if  (bitmask & (1 << PATCH))                   strcat(verbList, ",DELETE");
    if ((bitmask & (1 << OPTIONS)) && corsEnabled) strcat(verbList, ",OPTIONS");

    verbList = &verbList[1];  // Skipping first comma
  }
  
  return verbList;
}



#define NGSILD_ALLOW_HEADERS   "*"
#define NGSILD_EXPOSE_HEADERS  "*"



// ----------------------------------------------------------------------------
//
// orionldCorsOptions -
//
bool orionldCorsOptions(void)
{
  //
  // Either CORS is enabled in the broker or not, the list of allowed verbs is needed
  //
  char  verbListV[256];
  char* verbList;

  verbList = orionldAllowedVerbs(verbListV, sizeof(verbListV));


  //
  // Is CORS even enabled?
  //
  if (allowedOrigin[0] == 0)  // This I believe means that CORS is not enabled
  {
    orionldHeaderAdd(&orionldState.out.headers, HttpAllow, verbList, 0);
    orionldState.httpStatusCode = 405;
    return false;
  }


  // 
  //
  // Is the origin allowed for CORS ?
  //
  char* allowOrigin = NULL;
  if (orionldState.in.origin != NULL)
  {
    if      (strcmp(allowedOrigin, "__ALL")                == 0) allowOrigin = (char*) "*";
    else if (strcmp(allowedOrigin, orionldState.in.origin) == 0) allowOrigin = allowedOrigin;

    if (allowOrigin == NULL)
    {
      orionldError(OrionldOperationNotSupported, "Origin not allowed", orionldState.in.origin, 406);
      return false;
    }
  }

  // Access-Control-Allow-Origin
  orionldHeaderAdd(&orionldState.out.headers, HttpAllowOrigin, allowOrigin, 0);

  // Access-Control-Allow-Headers
  orionldHeaderAdd(&orionldState.out.headers, HttpAllowHeaders, NGSILD_ALLOW_HEADERS, 0);

  // Access-Control-Allow-Methods
  orionldHeaderAdd(&orionldState.out.headers, HttpAllowMethods, verbList, 0);

  // Access-Control-Max-Age
  orionldHeaderAdd(&orionldState.out.headers, HttpMaxAge, NULL, maxAge);

  // Access-Control-Expose-Headers
  orionldHeaderAdd(&orionldState.out.headers, HttpExposeHeaders, NGSILD_EXPOSE_HEADERS, 0);

  orionldState.httpStatusCode = 200;
  return true;
}
