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
#include "ngsi/ErrorCode.h"



/* ****************************************************************************
*
* ErrorCode::ErrorCode - 
*/
ErrorCode::ErrorCode()
{
  code         = NO_ERROR_CODE;
  reasonPhrase = "";
  details      = "";
}



/* ****************************************************************************
*
* ErrorCode::ErrorCode - 
*/
ErrorCode::ErrorCode(int _code, std::string _reasonPhrase, std::string _details)
{
  code          = _code;
  reasonPhrase  = _reasonPhrase;
  details       = _details;
}



/* ****************************************************************************
*
* ErrorCode::render - 
*/
std::string ErrorCode::render(Format format, std::string indent)
{
  std::string out     = "";
  std::string tag     = "errorCode";

  if (code == NO_ERROR_CODE)
  {
    code          = 500;
    reasonPhrase += " - ZERO code set to 500";
  }

  out += startTag(indent, tag, format);
  out += valueTag(indent + "  ", "code", code, format, true);
  out += valueTag(indent + "  ", "reasonPhrase", reasonPhrase, format, details != "");

  if (details != "")
     out += valueTag(indent + "  ", "details", details, format, false);

  out += endTag(indent, tag, format);

  return out;
}



/* ****************************************************************************
*
* ErrorCode::fill - 
*/
void ErrorCode::fill(int _code, std::string _reasonPhrase, std::string _details)
{
   code          = _code;
   reasonPhrase  = _reasonPhrase;
   details       = _details;
}

/* ****************************************************************************
*
* ErrorCode::fill - 
*/
void ErrorCode::fill(StatusCode* scP)
{
   code          = scP->code;
   reasonPhrase  = scP->reasonPhrase;
   details       = scP->details;
}

/* ****************************************************************************
*
* ErrorCode::fill -
*
* FIXME P3: having StatusCode and ErrorCode actually being the same type lead to this dirty duplication
*/
void ErrorCode::fill(ErrorCode* scP)
{
   code          = scP->code;
   reasonPhrase  = scP->reasonPhrase;
   details       = scP->details;
}


/* ****************************************************************************
*
* ErrorCode::check - 
*/
std::string ErrorCode::check(RequestType requestType, Format format, std::string indent, std::string predetectedError, int counter)
{
  if (code == 0)
    return "no code";

  if (reasonPhrase == "")
    return "no reason phrase";

  return "OK";
}



/* ****************************************************************************
*
* ErrorCode::present - 
*/
void ErrorCode::present(std::string indent)
{
   PRINTF("%sCode:            %d\n",   indent.c_str(), code);
   PRINTF("%sReasonPhrase:    '%s'\n", indent.c_str(), reasonPhrase.c_str());
   PRINTF("%sDetail:          '%s'\n", indent.c_str(), details.c_str());
}



/* ****************************************************************************
*
* ErrorCode::release - 
*/
void ErrorCode::release(void)
{
   /* This method is included for the sake of homogeneity */
}
