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

#include "common/string.h"
#include "common/JsonHelper.h"
#include "rest/ConnectionInfo.h"
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
void OrionError::fill(const OrionError& oe)
{
  fill(oe.code, oe.description, oe.error);
}



/* ****************************************************************************
*
* OrionError::fill -
*/
void OrionError::fill(OrionError* oeP)
{
  fill(oeP->code, oeP->description, oeP->error);
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
* OrionError::setSCAndRender -
*/
std::string OrionError::setSCAndRender(HttpStatusCode* scP)
{
  *scP = code;
  return toJson();
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

