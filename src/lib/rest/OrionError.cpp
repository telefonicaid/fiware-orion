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
#include "common/JsonHelper.h"
#include "rest/ConnectionInfo.h"
#include "ngsi/StatusCode.h"
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
  reasonPhrase  = _reasonPhrase.empty() ? httpStatusCodeString(code) : _reasonPhrase;
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
*/
void OrionError::fill(HttpStatusCode _code, const std::string& _details, const std::string& _reasonPhrase)
{
  code          = _code;
  reasonPhrase  = !_reasonPhrase.empty()? _reasonPhrase : httpStatusCodeString(code);
  details       = _details;
}



/* ****************************************************************************
*
* OrionError::fill -
*/
void OrionError::fill(const StatusCode& sc)
{
  code          = sc.code;
  reasonPhrase  = (!sc.reasonPhrase.empty())? sc.reasonPhrase : httpStatusCodeString(code);
  details       = sc.details;
}



/* ****************************************************************************
*
* OrionError::smartRender -
*/
std::string OrionError::smartRender(ApiVersion apiVersion)
{
  if (apiVersion == V1 || apiVersion == NO_VERSION)
  {
    return toJsonV1();
  }
  else // admin or v2
  {
    shrinkReasonPhrase();
    return toJson();
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

  return smartRender(apiVersion);
}



/* ****************************************************************************
*
* OrionError::toJson -
*/
std::string OrionError::toJson(void)
{
  char*  reasonPhraseEscaped = htmlEscape(reasonPhrase.c_str());
  char*  detailsEscaped      = htmlEscape(details.c_str());

  JsonObjectHelper jh;

  jh.addString("error", reasonPhraseEscaped);
  jh.addString("description", detailsEscaped);

  free(reasonPhraseEscaped);
  free(detailsEscaped);

  return jh.str();
}



/* ****************************************************************************
*
* OrionError::toJsonV1 -
*
*/
std::string OrionError::toJsonV1(void)
{
  std::string  out           = "{";

  //
  // OrionError is NEVER part of any other payload, so the JSON start/end braces must be added here
  //
  out += startTag("orionError", false);
  out += valueTag("code",          code,         true);
  out += valueTag("reasonPhrase",  reasonPhrase, !details.empty());

  if (!details.empty())
  {
    out += valueTag("details",       details);
  }

  out += endTag();

  out += "}";

  return out;
}



/* ****************************************************************************
*
* OrionError::shrinkReasonPhrase -
*
* This method removes any whitespace in the reasonPhrase field, i.e.
* transforms "Not Found" to "NotFound".
*
* It is used by smartRender method, in order to prepare to render in API v2 case
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
