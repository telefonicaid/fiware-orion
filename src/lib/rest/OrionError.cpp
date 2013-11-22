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
* fermin at tid dot es
*
* Author: Ken Zangelin
*/
#include <stdio.h>
#include <string>

#include "common/tag.h"
#include "common/Format.h"
#include "rest/OrionError.h"



/* ****************************************************************************
*
* OrionError::OrionError - 
*/
OrionError::OrionError()
{
  code         = 0;
  reasonPhrase = "";
  details      = "";
}



/* ****************************************************************************
*
* OrionError::OrionError - 
*/
OrionError::OrionError(int _code, std::string _reasonPhrase, std::string _details)
{
  code          = _code;
  reasonPhrase  = _reasonPhrase;
  details       = _details;
}



/* ****************************************************************************
*
* OrionError::OrionError - 
*/
OrionError::OrionError(ErrorCode& errorCode)
{
  code          = errorCode.code;
  reasonPhrase  = errorCode.reasonPhrase;
  details       = errorCode.details;
}



/* ****************************************************************************
*
* OrionError::OrionError - 
*/
OrionError::OrionError(StatusCode& statusCode)
{
  code          = statusCode.code;
  reasonPhrase  = statusCode.reasonPhrase;
  details       = statusCode.details;
}



/* ****************************************************************************
*
* OrionError::render - 
*/
std::string OrionError::render(Format format, std::string indent)
{
  std::string out     = "";
  std::string tag     = "orionError";

  out += startTag(indent, tag, format);
  out += valueTag(indent + "  ", "code",          code,         format, true);
  out += valueTag(indent + "  ", "reasonPhrase",  reasonPhrase, format, details != "");

  if (details != "")
    out += valueTag(indent + "  ", "details",       details,      format);

  out += endTag(indent, tag, format);

  return out;
}
