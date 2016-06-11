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

#include "common/tag.h"
#include "rest/ConnectionInfo.h"
#include "rest/OrionError.h"
#include "rest/errorAdaptation.h"



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
* OrionError::smartRender -
*/
std::string OrionError::smartRender(const std::string& apiVersion)
{
  if (apiVersion == "v1")
  {
    return render();
  }
  else // v2
  {
    shrinkReasonPhrase();
    return toJson();
  }
}



/* ****************************************************************************
*
* OrionError::toJson -
*/
std::string OrionError::toJson(void)
{
  return "{" + JSON_STR("error") + ":" + JSON_STR(reasonPhrase) + "," + JSON_STR("description") + ":" + JSON_STR(details) + "}";
}


#if 0
/* ****************************************************************************
*
* OrionError::render - 
*/
std::string OrionError::render(ConnectionInfo* ciP, const std::string& _indent)
{
  //
  // For API version 2 this is pretty easy ...
  //
  // FIXME: render() should not be used for v2, in favour of toJson(), so removing
  // this piece of code. Note that modifying ciP->httpStatusCode in a function aimed at
  // just rendering (i.e. printing string) is very dangerous and breaks separation of
  // concerns
  //
  if (ciP->apiVersion == "v2")
  {
    if ((ciP->httpStatusCode == SccOk) || (ciP->httpStatusCode == SccNone))
    {
      ciP->httpStatusCode = SccBadRequest;
    }

    if (details == "Already Exists")
    {
      details = "Entity already exists";
    }

    reasonPhrase = errorStringForV2(reasonPhrase);
    return "{" + JSON_STR("error") + ":" + JSON_STR(reasonPhrase) + "," + JSON_STR("description") + ":" + JSON_STR(details) + "}";
  }

  //
  // A little more hairy for API version 1
  //

  std::string  out           = "";
  std::string  tag           = "orionError";
  std::string  initialIndent = _indent;
  std::string  indent        = _indent;

  //
  // OrionError is NEVER part of any other payload, so the JSON start/end braces must be added here
  //


  out     = initialIndent + "{\n";
  indent += "  ";

  out += startTag1(indent, tag);
  out += valueTag(indent + "  ", "code",          code,         true);
  out += valueTag1(indent + "  ", "reasonPhrase",  reasonPhrase, details != "");

  if (details != "")
  {
    out += valueTag1(indent + "  ", "details",       details);
  }

  out += endTag(indent);

  out += initialIndent + "}\n";

  return out;
}
#endif


/* ****************************************************************************
*
* OrionError::render -
*
*/
std::string OrionError::render(void)
{
#if 0
  //
  // For API version 2 this is pretty easy ...
  //
  // FIXME: render() should not be used for v2, in favour of toJson(), so removing
  // this piece of code
  //
  if (apiVersion == "v2")
  {
    if (details == "Already Exists")
    {
      details = "Entity already exists";
    }

    reasonPhrase = errorStringForV2(reasonPhrase);
    return "{" + JSON_STR("error") + ":" + JSON_STR(reasonPhrase) + "," + JSON_STR("description") + ":" + JSON_STR(details) + "}";
  }
#endif


  //
  // A little more hairy for API version 1
  //

  std::string  out           = "";
  std::string  tag           = "orionError";
  std::string  initialIndent = "";
  std::string  indent        = "";

  //
  // OrionError is NEVER part of any other payload, so the JSON start/end braces must be added here
  //


  out     = initialIndent + "{\n";
  indent += "  ";

  out += startTag1(indent, tag);
  out += valueTag(indent  + "  ", "code",          code,         true);
  out += valueTag1(indent + "  ", "reasonPhrase",  reasonPhrase, details != "");

  if (details != "")
  {
    out += valueTag1(indent + "  ", "details",       details);
  }

  out += endTag(indent);

  out += initialIndent + "}\n";

  return out;
}



/* ****************************************************************************
*
* OrionError::render -
*
* This method removes any whitespace in the reasonPhrase field, i.e.
* transforms "Not Found" to "NotFound".
*
* It is used by smartRender method, in order to prepare to render in API v2 case
*
*/
void OrionError::shrinkReasonPhrase(void)
{
  char buf[80];  // 80 should be enough to hold any reason phrase

  strncpy(buf, reasonPhrase.c_str(), 80);

  // See: http://stackoverflow.com/questions/1726302/removing-spaces-from-a-string-in-c
  char* i = buf;
  char* j = buf;
  while(*j != 0)
  {
    *i = *j++;
    if(*i != ' ')
      i++;
  }
  *i = 0;

  reasonPhrase = std::string(buf);
}
