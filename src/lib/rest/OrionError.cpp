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
#include "common/Format.h"
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
OrionError::OrionError(HttpStatusCode _code, const std::string& _details)
{
  code          = _code;
  reasonPhrase  = httpStatusCodeString(code);
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
* OrionError::render - 
*/
std::string OrionError::render(Format format, const std::string& _indent)
{
  std::string out           = "";
  std::string tag           = "orionError";
  std::string initialIndent = _indent;
  std::string indent        = _indent;

  //
  // OrionError is NEVER part of any other payload, so the JSON start/end braces must be added here
  //

  if (format == JSON)
  {
    out     = initialIndent + "{\n";
    indent += "  ";
  }

  out += startTag(indent, tag, format);
  out += valueTag(indent + "  ", "code",          code,         format, true);
  out += valueTag(indent + "  ", "reasonPhrase",  reasonPhrase, format, details != "");

  if (details != "")
    out += valueTag(indent + "  ", "details",       details,      format);

  out += endTag(indent, tag, format);

  if (format == JSON)
    out += initialIndent + "}\n";

  return out;
}
