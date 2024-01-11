/*
*
* Copyright 2024 FIWARE Foundation e.V.
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
#include "logMsg/logMsg.h"                                // LM_RE

#include "orionld/common/orionldState.h"                  // orionldState



// -----------------------------------------------------------------------------
//
// forbidden -
//
bool forbidden(const char* s, const char* exceptions)
{
  bool bad = false;

  while (*s != 0)
  {
    if      (*s == '<')  bad = true;
    else if (*s == '>')  bad = true;
    else if (*s == '"')  bad = true;
    else if (*s == '\'') bad = true;
    else if (*s == '(')  bad = true;
    else if (*s == ')')  bad = true;

    if (orionldState.apiVersion == API_VERSION_NGSILD_V1)
    {
      if      (*s == ';')  bad = true;
      else if (*s == '=')  bad = true;
    }

    if (bad == true)
      LM_RE(true, ("Invalid character: 0x%x '%c'", *s & 0xFF, *s));

    ++s;
  }

  return false;
}
