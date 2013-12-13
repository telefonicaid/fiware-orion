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
#include "ngsi/Scope.h"
#include "common/Format.h"



/* ****************************************************************************
*
* Scope::Scope - 
*/
Scope::Scope()
{
  type  = "";
  value = "";
}



/* ****************************************************************************
*
* Scope::Scope - 
*/
Scope::Scope(std::string _type, std::string _value)
{
  type  = _type;
  value = _value;
}



/* ****************************************************************************
*
* Scope::render - 
*/
std::string Scope::render(Format format, std::string indent, bool notLastInVector)
{
  std::string out = "";
  std::string tag = "operationScope";

  out += startTag(indent, tag, tag, format, false, false);
  out += valueTag(indent + "  ", "type", type, format, true);
  out += valueTag(indent + "  ", "value", value, format);
  out += endTag(indent, tag, format, notLastInVector);

  return out;
}



/* ****************************************************************************
*
* Scope::check - 
*/
std::string Scope::check(RequestType requestType, Format format, std::string indent, std::string predetectedError, int counter)
{
  LM_T(LmtScope, ("type == '%s'", type.c_str()));

  if ((type == "") || (type == "not in use"))
    return "Empty type in restriction scope";

  if ((value == "") || (value == "not in use"))
    return "Empty value in restriction scope";

  return "OK";
}



/* ****************************************************************************
*
* Scope::present - 
*/
void Scope::present(std::string indent, int ix)
{
  if (ix == -1)
    PRINTF("%sScope:\n",       indent.c_str());
  else
    PRINTF("%sScope %d:\n",    indent.c_str(), ix);

  PRINTF("%s  Type:     %s\n", indent.c_str(), type.c_str());
  PRINTF("%s  Value:    %s\n", indent.c_str(), value.c_str());
}



/* ****************************************************************************
*
* release - 
*/
void Scope::release(void)
{
   /* This method is included for the sake of homogeneity */
}
