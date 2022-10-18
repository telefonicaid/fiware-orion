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
// validUriChars - valid characters in a URI (according to RFC 3986):
//
//   * %
//   * : / ? # [ ] @
//   * ! $ & ' ( ) * + , ; =
//   * A-Z
//   * a-z
//   * 0-9
//   * - . _ ~
//
static char validUriChars[256];



// -----------------------------------------------------------------------------
//
// pCheckUriInit - initialize the array of valid/invalid chars for URI
//
void pCheckUriInit(void)
{
  bzero(validUriChars, sizeof(validUriChars));

  // Setting VALID for A-Z
  for (int ix = 'A'; ix <= 'Z'; ix++)
  {
    validUriChars[ix] = true;
  }

  // Setting VALID for a-z
  for (int ix = 'a'; ix <= 'z'; ix++)
  {
    validUriChars[ix] = true;
  }

  // Setting VALID for 0-9
  for (int ix = '0'; ix <= '9'; ix++)
  {
    validUriChars[ix] = true;
  }

  // Special characters
  validUriChars['%']  = true;
  validUriChars[':']  = true;
  validUriChars['/']  = true;
  validUriChars['?']  = true;
  validUriChars['#']  = true;
  validUriChars['[']  = true;
  validUriChars[']']  = true;
  validUriChars['@']  = true;
  validUriChars['!']  = true;
  validUriChars['$']  = true;
  validUriChars['&']  = true;
  validUriChars['\''] = true;
  validUriChars['(']  = true;
  validUriChars[')']  = true;
  validUriChars['*']  = true;
  validUriChars['+']  = true;
  validUriChars[',']  = true;
  validUriChars[':']  = true;
  validUriChars['=']  = true;
  validUriChars['-']  = true;
  validUriChars['.']  = true;
  validUriChars['_']  = true;
  validUriChars['~']  = true;

  // Not so sure about this ... (MSB set)
  for (int ix = 0x80; ix <= 0xFF; ix++)
  {
    validUriChars[ix]  = true;
  }
}



// -----------------------------------------------------------------------------
//
// pCheckUri - check that the string 'uri' is a valid URI
//
bool pCheckUri(const char* uri, const char* name, bool mustBeUri)
{
  bool hasColon = false;

  if (uri == NULL)
  {
    orionldError(OrionldBadRequestData, "No URI", name, 400);
    return false;
  }
  else if (*uri == 0)
  {
    if (mustBeUri == true)
      orionldError(OrionldBadRequestData, "Empty URI", name, 400);
    else
      orionldError(OrionldBadRequestData, "Empty String", name, 400);

    return false;
  }


  //
  // Is there a colon somewhere inside the URI string?
  //
  char* s = (char*) uri;
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
    orionldError(OrionldBadRequestData, "Invalid URI - no colon found", name, 400);
    return false;
  }


  //
  // If not strict and no colon found - it's considered a shortname
  // For shortnames, we only check for the space character - it's forbidden
  //
  s = (char*) uri;
  if ((mustBeUri == false) && (hasColon == false))
  {
    while (*s != 0)
    {
      if (*s == ' ')
      {
        orionldError(OrionldBadRequestData, "Invalid URI/Shortname - whitespace present", name, 400);
        return false;
      }

      ++s;
    }
  }
  else
  {
    while (*s != 0)
    {
      if (validUriChars[(unsigned char) *s] == false)
      {
        orionldError(OrionldBadRequestData, "Invalid URI - invalid character", name, 400);
        LM_W(("Bad Input (invalid character in URI '%s', at position %d (0x%x)", uri, (int) (s - uri),  *s & 0xFF));
        return false;
      }

      ++s;
    }
  }

  return true;
}
