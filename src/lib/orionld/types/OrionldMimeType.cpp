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

#include "orionld/common/orionldState.h"                         // orionldState
#include "orionld/common/stringStrip.h"                          // stringStrip
#include "orionld/types/OrionldMimeType.h"                       // Own interface



/* ****************************************************************************
*
* mimeTypeFromString -
*/
MimeType mimeTypeFromString(const char* mimeType, char** charsetP, bool wildcardsAccepted, bool textOk, uint32_t* acceptMaskP)
{
  char* s;
  char* cP = (char*) mimeType;

  if ((cP == NULL) || (*cP == 0))
    return MT_NONE;

  if ((s = strstr(cP, ";")) != NULL)
  {
    *s = 0;
    ++s;
    s = stringStrip(s);

    if ((charsetP != NULL) && (strncmp(s, "charset=", 8) == 0))
      *charsetP = &s[8];
  }

  cP = stringStrip(cP);

  if (wildcardsAccepted == true)
  {
    if (strcmp(cP, "*/*") == 0)
    {
      *acceptMaskP = 0xFFFFFFFF;  // Accept anything, prefer JSON!
      return MT_JSON;
    }
    else if (strcmp(cP, "application/*") == 0)
    {
      if (orionldState.apiVersion == API_VERSION_NGSILD_V1)
        *acceptMaskP = (1 << MT_JSON) | (1 << MT_JSONLD) | (1 << MT_GEOJSON) | (1 << MT_MERGEPATCHJSON);
      else
        *acceptMaskP = (1 << MT_JSON) | (1 << MT_HTML);

      return MT_JSON;
    }
    else if (strcmp(cP, "text/*") == 0)
    {
      if (orionldState.apiVersion != API_VERSION_NGSILD_V1)
      {
        *acceptMaskP = (1 << MT_TEXT);
        return MT_TEXT;
      }
    }
  }

  if      (strcmp(cP, "application/json")             == 0)  return MT_JSON;
  else if (strcmp(cP, "application/ld+json")          == 0)  return MT_JSONLD;
  else if (strcmp(cP, "application/geo+json")         == 0)  return MT_GEOJSON;
  else if (strcmp(cP, "application/merge-patch+json") == 0)  return MT_MERGEPATCHJSON;

  if (orionldState.apiVersion != API_VERSION_NGSILD_V1)
  {
    if (strcmp(cP, "application/html")  == 0)  return MT_HTML;

    if (textOk)
    {
      if      (strcmp(cP, "text/json")  == 0)  return MT_JSON;
      else if (strcmp(cP, "text/plain") == 0)  return MT_TEXT;
    }
  }

  return MT_NONE;
}



// -----------------------------------------------------------------------------
//
// mimeType -
//
const char* mimeType(MimeType mimeType)
{
  switch (mimeType)
  {
  case MT_NONE:             return "None";
  case MT_NOTGIVEN:         return "NotGiven";
  case MT_JSON:             return "application/json";
  case MT_JSONLD:           return "application/ld+json";
  case MT_GEOJSON:          return "application/geo+json";
  case MT_TEXT:             return "text/plain";
  case MT_HTML:             return "application/html";
  case MT_MERGEPATCHJSON:   return "application/merge-patch+json";
  }

  return "NotFound";
}
