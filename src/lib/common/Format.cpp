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
#include "common/Format.h"
#include "common/wsStrip.h"



/* ****************************************************************************
*
* formatToString - 
*/
const char* formatToString(Format format)
{
  switch (format)
  {  
  case JSON:     return "JSON";
  case TEXT:     return "TEXT";
  case HTML:     return "HTML";
  case NOFORMAT: return "NOFORMAT";
  }

  return "Unknown format";
}



/* ****************************************************************************
*
* stringToFormat
*/
Format stringToFormat(const std::string& s)
{
  if (s == "JSON")
  {
    return JSON;
  }

  return NOFORMAT;
}



/* ****************************************************************************
*
* formatParse - 
*/
Format formatParse(const std::string& formatString, std::string* charsetP)
{
  char* s;
  char* cP = (char*) formatString.c_str();

  if ((s = strstr(cP, ";")) != NULL)
  {
    *s = 0;
    ++s;
    s = wsStrip(s);
    if (strncmp(s, "charset=", 8) == 0)
    {
      if (charsetP != NULL)
        *charsetP = std::string(&s[8]);
    }
  }

  std::string format(wsStrip(cP));

  if      (format == "*/*")                               return JSON;
  else if (format == "text/json")                         return JSON;
  else if (format == "application/json")                  return JSON;
  else if (format == "text/plain")                        return TEXT;

  return JSON;
}
