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
#include <string.h>                                             // strspn, strlen
#include <unistd.h>                                             // NULL

#include "logMsg/logMsg.h"                                      // LM_*
#include "logMsg/traceLevels.h"                                 // Lmt*

#include "orionld/payloadCheck/pcheckUri.h"                     // Own interface



// -----------------------------------------------------------------------------
//
// valid - valid characters in a URI (according to RFC 3986):
//
//   * %
//   * : / ? # [ ] @
//   * ! $ & ' ( ) * + , ; =
//   * A-Z
//   * a-z
//   * 0-9
//   * - . _ ~
//
static char valid[256];



// -----------------------------------------------------------------------------
//
// pcheckUriInit - initialize the array of valid/invalid chars for URI
//
void pcheckUriInit(void)
{
  bzero(valid, sizeof(valid));

  // Setting VALID for A-Z
  for (int ix = 'A'; ix <= 'Z'; ix++)
  {
    valid[ix] = true;
  }

  // Setting VALID for a-z
  for (int ix = 'a'; ix <= 'z'; ix++)
  {
    valid[ix] = true;
  }

  // Setting VALID for 0-9
  for (int ix = '0'; ix <= '9'; ix++)
  {
    valid[ix] = true;
  }

  // And at last, the special charcters
  valid['%']  = true;
  valid[':']  = true;
  valid['/']  = true;
  valid['?']  = true;
  valid['#']  = true;
  valid['[']  = true;
  valid[']']  = true;
  valid['@']  = true;
  valid['!']  = true;
  valid['$']  = true;
  valid['&']  = true;
  valid['\''] = true;
  valid['(']  = true;
  valid[')']  = true;
  valid['*']  = true;
  valid['+']  = true;
  valid[',']  = true;
  valid[':']  = true;
  valid['=']  = true;
  valid['-']  = true;
  valid['.']  = true;
  valid['_']  = true;
  valid['~']  = true;
}



// -----------------------------------------------------------------------------
//
// pcheckUri - check that the string 'uri' is a valid URI
//
// If strict - cannot be a shoprtname
// If colon found - it's not a shortname
//
// If shortname - all chars are OK except ' '
//
bool pcheckUri(char* uri, bool strict, char** detailP)
{
  bool hasColon = false;

  if (uri == NULL)
  {
    *detailP = (char*) "NULL URI";
    return false;
  }
  else if (*uri == 0)
  {
    *detailP = (char*) "Empty URI";
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
  // If it's a strict URI - there must be a colon present
  //
  if ((strict == true) && (hasColon == false))
  {
    *detailP = (char*) "Invalid URI: no colon";
    LM_W(("Bad Input (not a URI - no colon found in '%s'", uri));
    return false;
  }


  //
  // If not strict and no colon found - it's considered a shortname
  // For shortnames, we only check for the space character - it's forbidden
  //
  if ((strict == false) && (hasColon == false))
  {
    s = uri;

    while (*s != 0)
    {
      if (*s == ' ')
      {
        *detailP = (char*) "space in shortname";
        LM_W(("Bad Input (space in shortname '%s' at position %d", uri, (int) (s - uri)));
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
      if (valid[(unsigned char) *s] == false)
      {
        *detailP = (char*) "invalid character in URI";
        LM_W(("Bad Input (invalid character in URI '%s', at position %d (0x%x)", uri, (int) (s - uri),  *s & 0xFF));
        return false;
      }

      ++s;
    }
  }

  return true;
}
