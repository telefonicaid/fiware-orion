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
#include <string>

#include "Verb.h"



/* ****************************************************************************
*
* verbName - 
*/
const char* verbName(Verb verb)
{
  switch (verb)
  {
  case HTTP_GET:         return "GET";
  case HTTP_PUT:         return "PUT";
  case HTTP_POST:        return "POST";
  case HTTP_DELETE:      return "DELETE";
  case HTTP_PATCH:       return "PATCH";
  case HTTP_HEAD:        return "HEAD";
  case HTTP_OPTIONS:     return "OPTIONS";
  case HTTP_TRACE:       return "TRACE";
  case HTTP_CONNECT:     return "CONNECT";
  case HTTP_NOVERB:      return "";            // Should not be rendered at all
  case HTTP_UNKNOWNVERB: return "UNKNOWNVERB"; // Should not progress away parsing
  }

  return "Unknown verb";
}



/* ****************************************************************************
*
* str2Verb -
*/
Verb str2Verb(const std::string& str)
{
  if      (str == "GET")     return HTTP_GET;
  else if (str == "PUT")     return HTTP_PUT;
  else if (str == "POST")    return HTTP_POST;
  else if (str == "DELETE")  return HTTP_DELETE;
  else if (str == "PATCH")   return HTTP_PATCH;
  else if (str == "HEAD")    return HTTP_HEAD;
  else if (str == "OPTIONS") return HTTP_OPTIONS;
  else if (str == "TRACE")   return HTTP_TRACE;
  else if (str == "CONNECT") return HTTP_CONNECT;
  else if (str == "")        return HTTP_NOVERB;

  return HTTP_UNKNOWNVERB;
}
