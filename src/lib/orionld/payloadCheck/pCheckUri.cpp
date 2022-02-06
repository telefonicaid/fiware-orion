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
#include <unistd.h>                                             // NULL

#include "logMsg/logMsg.h"                                      // LM_*

#include "orionld/common/orionldError.h"                        // orionldError
#include "orionld/payloadCheck/pCheckUri.h"                     // Own interface



// -----------------------------------------------------------------------------
//
// validUriChars -
//
extern char validUriChars[256];  // From pcheckUri.cpp



// -----------------------------------------------------------------------------
//
// pCheckUri - newer version of pcheckUri - accepting OrionldProblemDetails as input/output
//
bool pCheckUri(char* uri, bool mustBeUri, OrionldProblemDetails* pdP)
{
  bool hasColon = false;

  if (uri == NULL)
  {
    if (pdP != NULL)
      orionldError(pdP, OrionldBadRequestData, "No URI", NULL, 400);
    LM_W(("Bad Input (no URI)"));
    return false;
  }
  else if (*uri == 0)
  {
    if (pdP != NULL)
      orionldError(pdP, OrionldBadRequestData, "Empty URI", NULL, 400);
    LM_W(("Bad Input (Empty URI)"));
    return false;
  }


  //
  // Is there a colon somewhere inside the URI string?
  //
  char* s = uri;
  while (*s != 0)
  {
    if (*s == ':')
      hasColon = true;
    ++s;
  }

  //
  // If it's a strict URI (mustBeUri == TRUE) - there must be a colon present
  //
  if ((mustBeUri == true) && (hasColon == false))
  {
    if (pdP != NULL)
      orionldError(pdP, OrionldBadRequestData, "Invalid URI", "No colon present", 400);
    LM_W(("Bad Input (%s: %s)", "Invalid URI", "No colon present"));
    return false;
  }


  //
  // If not strict and no colon found - it's considered a shortname
  // For shortnames, we only check for the space character - it's forbidden
  //
  if ((mustBeUri == false) && (hasColon == false))
  {
    s = uri;

    while (*s != 0)
    {
      if (*s == ' ')
      {
        if (pdP != NULL)
          orionldError(pdP, OrionldBadRequestData, "Invalid URI/Shortname", "whitespace in shortname", 400);
        LM_W(("Bad Input (%s: %s)", "Invalid URI/Shortname", "whitespace in shortname"));
        return false;
      }

      ++s;
    }
  }
  else
  {
    s = uri;

    while (*s != 0)
    {
      if (validUriChars[(unsigned char) *s] == false)
      {
        if (pdP != NULL)
          orionldError(pdP, OrionldBadRequestData, "Invalid URI", "invalid character", 400);
        LM_W(("Bad Input (invalid character in URI '%s', at position %d (0x%x)", uri, (int) (s - uri),  *s & 0xFF));
        return false;
      }

      ++s;
    }
  }

  return true;
}
