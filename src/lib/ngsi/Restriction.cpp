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
#include <string>

#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"

#include "common/globals.h"
#include "common/tag.h"
#include "ngsi/Request.h"
#include "ngsi/Restriction.h"



/* ****************************************************************************
*
* Restriction::check - 
*/
std::string Restriction::check(RequestType requestType, Format format, std::string indent, std::string predetectedError, int counter)
{
  std::string res;

  LM_T(LmtRestriction, ("In Restriction::check"));
  if (counter == 0) // Restriction is always optional
  {
    LM_T(LmtRestriction, ("Restriction::check returns OK (counter == 0)"));
    return "OK";
  }

  if ((res = scopeVector.check(requestType, format, indent, predetectedError,  counter)) != "OK")
  {
    LM_T(LmtRestriction, ("Restriction::check returns '%s'", res.c_str()));
    return res;
  }

  LM_T(LmtRestriction, ("Restriction::check returns OK (2)"));
  return "OK";
}



/* ****************************************************************************
*
* Restriction::present - 
*/
void Restriction::present(std::string indent)
{
  attributeExpression.present(indent);
  scopeVector.present(indent);
}



/* ****************************************************************************
*
* Restriction::render - 
*/
std::string Restriction::render(Format format, std::string indent)
{
  std::string  tag = "restriction";
  std::string  out = "";

  out += startTag(indent, tag, format);
  out += attributeExpression.render(format, indent + "  ");
  out += scopeVector.render(format, indent + "  ");
  out += endTag(indent, tag, format);

  return out;
}



/* ****************************************************************************
*
* Restriction::release - 
*/
void Restriction::release(void)
{
   attributeExpression.release();
   scopeVector.release();
}
