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

#include "common/globals.h"
#include "common/string.h"
#include "common/tag.h"
#include "ngsi/Request.h"
#include "common/Format.h"
#include "rest/HttpStatusCode.h"
#include "ngsi/StatusCode.h"



/* ****************************************************************************
*
* StatusCode::StatusCode - 
*/
StatusCode::StatusCode()
{
  code         = SccNone;
  reasonPhrase = "";
  details      = "";
  tag          = "statusCode";
}



/* ****************************************************************************
*
* StatusCode::StatusCode - 
*/
StatusCode::StatusCode(std::string _tag)
{
  code         = SccNone;
  reasonPhrase = "";
  details      = "";
  tag          = _tag;
}



/* ****************************************************************************
*
* StatusCode::StatusCode - 
*/
StatusCode::StatusCode(HttpStatusCode _code, std::string _details, std::string _tag)
{
  code          = _code;
  reasonPhrase  = httpStatusCodeString(code);
  details       = _details;
  tag           = _tag;
}


/* ****************************************************************************
*
* StatusCode::render - 
*/
std::string StatusCode::render(Format format, std::string indent, bool comma, bool showTag)
{
  std::string out  = "";

  if (code == SccNone)
  {
    fill(SccReceiverInternalError, "");
    details += " - ZERO code set to 500";
  }

  out += startTag(indent, tag, format, showTag);
  out += valueTag(indent + "  ", "code", code, format, true);
  out += valueTag(indent + "  ", "reasonPhrase", reasonPhrase, format, details != "");

  if (details != "")
     out += valueTag(indent + "  ", "details", details, format, false);

  out += endTag(indent, tag, format, comma);

  return out;
}



/* ****************************************************************************
*
* StatusCode::fill - 
*/
void StatusCode::fill(HttpStatusCode _code, std::string _details)
{
   code          = _code;
   reasonPhrase  = httpStatusCodeString(code);
   details       = _details;
}



/* ****************************************************************************
*
* StatusCode::fill -
*/
void StatusCode::fill(StatusCode* scP)
{
   fill(scP->code, scP->details);
}



/* ****************************************************************************
*
* StatusCode::check - 
*/
std::string StatusCode::check(RequestType requestType, Format format, std::string indent, std::string predetectedError, int counter)
{
  if (code == SccNone)
    return "no code";

  if (reasonPhrase == "")
    return "no reason phrase";

  return "OK";
}



/* ****************************************************************************
*
* StatusCode::present - 
*/
void StatusCode::present(std::string indent)
{
   PRINTF("%sCode:            %d",   indent.c_str(), code);
   PRINTF("%sReasonPhrase:    '%s'", indent.c_str(), reasonPhrase.c_str());
   PRINTF("%sDetail:          '%s'", indent.c_str(), details.c_str());
   PRINTF("%sTag:             '%s'", indent.c_str(), tag.c_str());
}



/* ****************************************************************************
*
* release - 
*/
void StatusCode::release(void)
{
  code         = SccNone;
  reasonPhrase = "";
  details      = "";
}



/* ****************************************************************************
*
* tagSet - 
*/
void StatusCode::tagSet(std::string _tag)
{
  tag = _tag;
}
