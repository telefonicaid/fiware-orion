/*
*
* Copyright 2023 FIWARE Foundation e.V.
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
#include <string.h>                                // strcmp

#include "orionld/types/Verb.h"                    // Verb + Own interface



// -----------------------------------------------------------------------------
//
// verbToString -
//
const char* verbToString(Verb verb)
{
  switch (verb)
  {
  case HTTP_UNKNOWNVERB:  return "Unknown";
  case HTTP_GET:          return "GET";
  case HTTP_PUT:          return "PUT";
  case HTTP_POST:         return "POST";
  case HTTP_DELETE:       return "DELETE";
  case HTTP_PATCH:        return "PATCH";
  case HTTP_HEAD:         return "HEAD";
  case HTTP_OPTIONS:      return "OPTIONS";
  case HTTP_TRACE:        return "TRACE";
  case HTTP_CONNECT:      return "CONNECT";
  case HTTP_NOVERB:       return "NOVERB";
  }

  return "Invalid Verb";
}



// -----------------------------------------------------------------------------
//
// verbFromString -
//
Verb verbFromString(const char* verb)
{
  if      (strcmp(verb, "GET")     == 0)  return HTTP_GET;
  else if (strcmp(verb, "POST")    == 0)  return HTTP_POST;
  else if (strcmp(verb, "PUT")     == 0)  return HTTP_PUT;
  else if (strcmp(verb, "PATCH")   == 0)  return HTTP_PATCH;
  else if (strcmp(verb, "DELETE")  == 0)  return HTTP_DELETE;
  else if (strcmp(verb, "HEAD")    == 0)  return HTTP_HEAD;
  else if (strcmp(verb, "OPTIONS") == 0)  return HTTP_OPTIONS;
  else if (strcmp(verb, "TRACE")   == 0)  return HTTP_TRACE;
  else if (strcmp(verb, "CONNECT") == 0)  return HTTP_CONNECT;

  return HTTP_UNKNOWNVERB;
}
