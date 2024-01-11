/*
*
* Copyright 2013 Telefonica Investigacion y Desarrollo, S.A.U
*
* This file is part of Orion Context Broker.
*
* Orion Context Broker is free software: you can redistribute it and/or
* modify it under the terms of the GNU Affero General Public License as
* published by the Free Software Foundation, either version 3 of the
* License, or (at your option) any later version.
*
* Orion Context Broker is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero
* General Public License for more details.
*
* You should have received a copy of the GNU Affero General Public License
* along with Orion Context Broker. If not, see http://www.gnu.org/licenses/.
*
* For those usages not covered by this license please contact with
* iot_support at tid dot es
*
* Author: Ken Zangelin
*/
#include <string.h>
#include <string>
#include <sstream>

#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"

#include "common/MimeType.h"



/* ****************************************************************************
*
* mimeTypeToString -
*/
const char* mimeTypeToString(MimeType mimeType)
{
  switch (mimeType)
  {
  case MT_JSON:            return "JSON";
  case MT_JSONLD:          return "JSONLD";
  case MT_GEOJSON:         return "GEOJSON";
  case MT_TEXT:            return "TEXT";
  case MT_HTML:            return "HTML";
  case MT_MERGEPATCHJSON:  return "MERGEPATCHJSON";
  case MT_NONE:            return "NOMIMETYPE";
  case MT_NOTGIVEN:        return "NOMIMETYPEGIVEN";
  }

  return "Unknown mimeType";
}



/* ****************************************************************************
*
* stringToMimeType
*/
MimeType stringToMimeType(const std::string& s)
{
  if      (s == "JSON")            return MT_JSON;
  else if (s == "JSONLD")          return MT_JSONLD;
  else if (s == "GEOJSON")         return MT_GEOJSON;
  else if (s == "TEXT")            return MT_TEXT;
  else if (s == "HTML")            return MT_HTML;
  else if (s == "MERGEPATCHJSON")  return MT_MERGEPATCHJSON;
  else if (s == "NOMIMETYPE")      return MT_NONE;
  else if (s == "NOMIMETYPEGIVEN") return MT_NOTGIVEN;

  return MT_NONE;
}


#ifdef ORIONLD
/* ****************************************************************************
*
* longStringToMimeType
*/
MimeType longStringToMimeType(const char* s)
{
  if      (strcmp(s, "application/json")             == 0) return MT_JSON;
  else if (strcmp(s, "application/ld+json")          == 0) return MT_JSONLD;
  else if (strcmp(s, "application/geo+json")         == 0) return MT_GEOJSON;
  else if (strcmp(s, "text/plain")                   == 0) return MT_TEXT;
  else if (strcmp(s, "application/html")             == 0) return MT_HTML;
  else if (strcmp(s, "application/merge-patch+json") == 0) return MT_MERGEPATCHJSON;
  else if (strcmp(s, "NOMIMETYPE")                   == 0) return MT_NONE;

  return MT_NONE;
}
#endif



/* ****************************************************************************
*
* mimeTypeToLongString -
*/
const char* mimeTypeToLongString(MimeType mimeType)
{
  switch (mimeType)
  {
  case MT_JSON:             return "application/json";
  case MT_JSONLD:           return "application/ld+json";
  case MT_GEOJSON:          return "application/geo+json";
  case MT_TEXT:             return "text/plain";
  case MT_HTML:             return "application/html";
  case MT_MERGEPATCHJSON:   return "application/merge-patch+json";
  case MT_NONE:             return "NOMIMETYPE";
  case MT_NOTGIVEN:         return "NOMIMETYPEGIVEN";
  }

  return "NOMIMETYPE";
}
