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
#include <string.h>                                              // strstr, strncmp, strcmp

#include "logMsg/logMsg.h"                                       // LM_*

#include "common/MimeType.h"                                     // MimeType
#include "common/wsStrip.h"                                      // wsStrip

#include "orionld/common/orionldState.h"                         // orionldState
#include "orionld/common/mimeTypeFromString.h"                   // Own interface



/* ****************************************************************************
*
* mimeTypeFromString -
*/
MimeType mimeTypeFromString(const char* mimeType, char** charsetP, bool wildcardsAccepted, bool textOk, uint32_t* acceptMaskP)
{
  char* s;
  char* cP = (char*) mimeType;

  LM_TMP(("KZ: Accept: '%s'", mimeType));
  if ((s = strstr(cP, ";")) != NULL)
  {
    *s = 0;
    ++s;
    s = wsStrip(s);

    if ((charsetP != NULL) && (strncmp(s, "charset=", 8) == 0))
      *charsetP = &s[8];
  }

  cP = wsStrip(cP);

  if (wildcardsAccepted == true)
  {
    if (strcmp(cP, "*/*") == 0)
    {
      *acceptMaskP = 0xFFFFFFFF;  // Accept anything, prefer JSON!
      return JSON;
    }
    else if (strcmp(cP, "application/*") == 0)
    {
      if (orionldState.apiVersion == NGSI_LD_V1)
        *acceptMaskP = (1 << JSON) | (1 << JSONLD) | (1 << GEOJSON) | (1 << MERGEPATCHJSON);
      else
        *acceptMaskP = (1 << JSON) | (1 << HTML);

      return JSON;
    }
    else if (strcmp(cP, "text/*") == 0)
    {
      if (orionldState.apiVersion != NGSI_LD_V1)
      {
        *acceptMaskP = (1 << TEXT);
        return TEXT;
      }
    }
  }

  if      (strcmp(cP, "application/json")             == 0)  return JSON;
  else if (strcmp(cP, "application/ld+json")          == 0)  return JSONLD;
  else if (strcmp(cP, "application/geo+json")         == 0)  return GEOJSON;
  else if (strcmp(cP, "application/merge-patch+json") == 0)  return MERGEPATCHJSON;

  if (orionldState.apiVersion != NGSI_LD_V1)
  {
    if (strcmp(cP, "application/html")  == 0)  return HTML;

    if (textOk)
    {
      if      (strcmp(cP, "text/json")  == 0)  return JSON;
      else if (strcmp(cP, "text/plain") == 0)  return TEXT;
    }
  }

  return NOMIMETYPE;
}
