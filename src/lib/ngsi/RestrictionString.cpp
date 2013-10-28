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
#include "ngsi/RestrictionString.h"



/* ****************************************************************************
*
* RestrictionString::check - 
*/
std::string RestrictionString::check(RequestType requestType, Format format, std::string indent, std::string predetectedError, int counter)
{
  return "OK";
}



/* ****************************************************************************
*
* RestrictionString::isEmpty - 
*/
bool RestrictionString::isEmpty(void)
{
   return (string == "")? true : false;
}



/* ****************************************************************************
*
* RestrictionString::set - 
*/
void RestrictionString::set(std::string value)
{
  string = value;
}



/* ****************************************************************************
*
* RestrictionString::get - 
*/
std::string RestrictionString::get(void)
{
  return string;
}



/* ****************************************************************************
*
* RestrictionString::present - 
*/
void RestrictionString::present(std::string indent)
{
  if (string != "")
    PRINTF("%sRestrictionString: %s\n", indent.c_str(), string.c_str());
  else
    PRINTF("%sNo RestrictionString\n", indent.c_str());
}



/* ****************************************************************************
*
* RestrictionString::render - 
*/
std::string RestrictionString::render(Format format, std::string indent)
{
  if (string == "")
    return "";

  return valueTag(indent, "restriction", string, format);
}



/* ****************************************************************************
*
* RestrictionString::c_str - 
*/
const char* RestrictionString::c_str(void)
{
   return string.c_str();
}
