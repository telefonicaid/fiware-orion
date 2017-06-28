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
#include <stdio.h>
#include <string>

#include <boost/lexical_cast.hpp>

#include "rest/ConnectionInfo.h"
#include "rest/OrionError.h"



/* ****************************************************************************
*
* OrionError::OrionError - 
*/
OrionError::OrionError()
{
  code         = SccNone;
  reasonPhrase = "";
  details      = "";
}



/* ****************************************************************************
*
* OrionError::OrionError -
*/
OrionError::OrionError(HttpStatusCode _code, const std::string& _details, const std::string& _reasonPhrase)
{
  code          = _code;
  reasonPhrase  = _reasonPhrase == "" ? httpStatusCodeString(code) : _reasonPhrase;
  details       = _details;
}



/* ****************************************************************************
*
* OrionError::OrionError - 
*/
OrionError::OrionError(StatusCode& sc)
{
  code          = sc.code;
  reasonPhrase  = httpStatusCodeString(code);
  details       = sc.details;
}



/* ****************************************************************************
*
* OrionError::fill - 
*
*/
void OrionError::fill(HttpStatusCode _code, const std::string& _details, const std::string& _reasonPhrase)
{
  code          = _code;
  reasonPhrase  = _reasonPhrase != ""? _reasonPhrase : httpStatusCodeString(code);
  details       = _details;
}



/* ****************************************************************************
*
* OrionError::render -
*/
std::string OrionError::render(ApiVersion apiVersion)
{
  if (apiVersion == V1 || apiVersion == NO_VERSION)
  {
    return renderV1();
  }
  else // admin or v2
  {
    shrinkReasonPhrase();
    return render();
  }
}



/* ****************************************************************************
*
* OrionError::setStatusCodeAndSmartRender -
*/
std::string OrionError::setStatusCodeAndSmartRender(ApiVersion apiVersion, HttpStatusCode* scP)
{
  if ((apiVersion == V2) || (apiVersion == ADMIN_API))
  {
    *scP = code;
  }

  return render(apiVersion);
}


/* ****************************************************************************
*
* OrionError::toJson -
*/
void OrionError::toJson
(
  JsonHelper& writer
)
{
  writer.StartObject();
  writer.String("error", reasonPhrase);
  writer.String("description", details);
  writer.EndObject();
}

/* ****************************************************************************
*
* OrionError::render -
*/
std::string OrionError::render(int indent)
{
  if (indent < 0)
  {
    indent = DEFAULT_JSON_INDENT;
  }
  JsonHelper writer(indent);

  toJson(writer);

  return writer.str();
}



/* ****************************************************************************
*
* OrionError::renderV1 -
*
*/
std::string OrionError::renderV1(int indent)
{
  if (indent < 0)
  {
    indent = DEFAULT_JSON_INDENT_V1;
  }
  JsonHelper writer(indent);

  //
  // OrionError is NEVER part of any other payload, so the JSON start/end braces must be added here
  //
  writer.StartObject();

  writer.StartObject("orionError");
  writer.String("code", boost::lexical_cast<std::string>(code));
  writer.String("reasonPhrase", reasonPhrase);

  if (details != "")
  {
    writer.String("details", details);
  }

  writer.EndObject();
  writer.EndObject();

  return writer.str();
}



/* ****************************************************************************
*
* OrionError::shrinkReasonPhrase -
*
* This method removes any whitespace in the reasonPhrase field, i.e.
* transforms "Not Found" to "NotFound".
*
* It is used by render method, in order to prepare to render in API v2 case
*
* FIXME P4: The following alternative (more compact) way has been proposed:
*
*  #include <algorithm>  // std::remove_if
*  #include <cctype>     // std::isspace
*
*  ...
*
*  reasonPhrase.erase(std::remove_if(reasonPhrase.begin(), reasonPhrase.end(), std::isspace), reasonPhrase.end());
*
* However, 'std::isspace' doesn't directly work. We have been able to make it work with
* 'static_cast<int(*)(int)>(isspace)'. However, that is obscure so until we can find
* a way of using just 'std::isspace', the current implementation stills.
*
*/
void OrionError::shrinkReasonPhrase(void)
{
  char buf[80];  // 80 should be enough to hold any reason phrase

#if 0
  strncpy(buf, reasonPhrase.c_str(), sizeof(buf));

  // See: http://stackoverflow.com/questions/1726302/removing-spaces-from-a-string-in-c
  if (*j != ' ')
  {
    *i = *j;
    ++i;
  }
  ++j;

  char* i = buf;
  char* j = buf;
  while (*j != 0)
  {
    *i = *j++;
    if (*i != ' ')
    {
      i++;
    }
  }
  *i = 0;
#endif

  char*         fromP = (char*) reasonPhrase.c_str();
  char*         toP   = buf;
  unsigned int  toLen = 0;

  while ((*fromP != 0) && (toLen < sizeof(buf)))
  {
    // If next char is not whitespace: copy to outgoing
    if (*fromP != ' ')
    {
      *toP = *fromP;
      ++toP;
      ++toLen;
    }

    ++fromP;
  }
  *toP = 0;  // End-of string

  reasonPhrase = std::string(buf);
}
