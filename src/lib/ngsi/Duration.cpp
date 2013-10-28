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
#include <string.h>   // strdup
#include <stdlib.h>   // free

#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"

#include "common/globals.h"
#include "common/tag.h"
#include "ngsi/Request.h"
#include "ngsi/Duration.h"

/* ****************************************************************************
*
* Duration::check - 
*/
std::string Duration::check(RequestType requestType, Format format, std::string indent, std::string predetectedError, int counter)
{
  if (string == "")
    return "OK";

  if (parse() == -1)
     return "syntax error in duration string";

  return "OK";
}



/* ****************************************************************************
*
* Duration::set - 
*/
void Duration::set(std::string value)
{
  string = value;
}



/* ****************************************************************************
*
* Duration::get - 
*/
std::string Duration::get(void)
{
  return string;
}

/* ****************************************************************************
*
* Duration::isEmpty -
*/
bool Duration::isEmpty(void)
{
  if ((string == "") || (string == "not in use"))
    return true;

  return false;
}


/* ****************************************************************************
*
* Duration::parse - 
*/
int Duration::parse(void)
{
  seconds = parse8601(string);
  return seconds;
}

/* ****************************************************************************
*
* Duration::present - 
*/
void Duration::present(std::string indent)
{
  if (string != "")
    PRINTF("%sDuration: %s\n", indent.c_str(), string.c_str());
  else
     PRINTF("%sNo Duration\n", indent.c_str());
}



/* ****************************************************************************
*
* Duration::render - 
*/
std::string Duration::render(Format format, std::string indent, bool comma)
{
  if (string == "")
    return "";

  return valueTag(indent, "duration", string, format, comma);
}



/* ****************************************************************************
*
* release - 
*/
void Duration::release(void)
{
   /* This method is included for the sake of homogeneity */
}
