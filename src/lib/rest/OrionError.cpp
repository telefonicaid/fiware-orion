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
  code        = SccNone;
  error       = "";
  description = "";
  filled      = false;
}



/* ****************************************************************************
*
* OrionError::OrionError -
*/
OrionError::OrionError(HttpStatusCode _code, const std::string& _description, const std::string& _error)
{
  code        = _code;
  error       = _error.empty() ? httpStatusCodeString(code) : _error;
  description = _description;
  filled      = true;
}



/* ****************************************************************************
*
* OrionError::OrionError -
*/
OrionError::OrionError(StatusCode& sc)
{
  code        = sc.code;
  error       = httpStatusCodeString(code);
  description = sc.details;
  filled      = true;
}



/* ****************************************************************************
*
* OrionError::fill -
*/
void OrionError::fill(HttpStatusCode _code, const std::string& _description, const std::string& _error)
{
  code        = _code;
  error       = _error.empty()? httpStatusCodeString(code) : _error;
  description = _description;
  filled      = true;
}



/* ****************************************************************************
*
* OrionError::fill -
*/
void OrionError::fill(const StatusCode& sc)
{
  code        = sc.code;
  error       = (sc.reasonPhrase.empty())? httpStatusCodeString(code) : sc.reasonPhrase;
  description = sc.details;
  filled      = true;
}



/* ****************************************************************************
*
* OrionError::fillOrAppend -
*/
void OrionError::fillOrAppend(HttpStatusCode _code, const std::string& fullDetails, const std::string& appendDetail, const std::string& _error)
{
  if (filled)
  {
    // Already filled by a previous operation. This can happen in batch update processing
    description += appendDetail;
  }
  else
  {
    code        = _code;
    error       = _error.empty()? httpStatusCodeString(code) : _error;
    description = fullDetails;
    filled      = true;
  }
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
    shrinkError();
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
  char*  reasonPhraseEscaped = htmlEscape(error.c_str());
  char*  detailsEscaped      = htmlEscape(description.c_str());

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
  out += startTag("orionError",   false);
  out += valueTag("code",         code, true);
  out += valueTag("reasonPhrase", error, !description.empty());

  if (!description.empty())
  {
    out += valueTag("details",    description);
  }

  out += endTag();

  out += "}";

  return out;
}



/* ****************************************************************************
*
* OrionError::shrinkError -
*
* This method removes any whitespace in the error field, i.e.
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
*  reasonPhrase.erase(std::remove_if(error.begin(), error.end(), std::isspace), error.end());
*
* However, 'std::isspace' doesn't directly work. We have been able to make it work with
* 'static_cast<int(*)(int)>(isspace)'. However, that is obscure so until we can find
* a way of using just 'std::isspace', the current implementation stills.
*
*/
void OrionError::shrinkError(void)
{
  char buf[80];  // 80 should be enough to hold any reason phrase

#if 0
  strncpy(buf, error.c_str(), sizeof(buf));

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

  char*         fromP = (char*) error.c_str();
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

  error = std::string(buf);
}
