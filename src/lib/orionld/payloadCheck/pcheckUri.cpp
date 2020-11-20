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
#include <unistd.h>                                               // NULL

#include "orionld/payloadCheck/pcheckUri.h"                       // Own interface



// -----------------------------------------------------------------------------
//
// pcheckUri - check that the string 's' contains a valid URI
//
bool pcheckUri(char* uri, char** detailP)
{
  if (uri == NULL)
  {
    *detailP = (char*) "NULL value";
    return false;
  }

  if (*uri == 0)
  {
    *detailP = (char*) "Empty value";
    return false;
  }

  while (*uri != 0)
  {
    if (*uri == ':')
      return true;

    if ((*uri >= 'A') && (*uri <= 'Z'))
    {
      // OK
    }
    else if ((*uri >= 'a') && (*uri <= 'z'))
    {
      // OK
    }
    else
    {
      *detailP = (char*) "Invalid character for the URI scheme";
      return false;
    }

    ++uri;
  }

  *detailP = (char*) "No colon found in URI";
  return false;
}
