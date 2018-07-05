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
#include <string>

#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"

#include "common/globals.h"
#include "common/tag.h"
#include "ngsi/Request.h"
#include "ngsi/AttributeDomainName.h"



/* ****************************************************************************
*
* AttributeDomainName::check - 
*/
std::string AttributeDomainName::check(void)
{
  // FIXME P3: AttributeDomainName::check always returns "OK"
  return "OK";
}



/* ****************************************************************************
*
* AttributeDomainName::isEmpty - 
*/
bool AttributeDomainName::isEmpty(void)
{
  if (string == "")
  {
    return true;
  }

  return false;
}



/* ****************************************************************************
*
* AttributeDomainName::set - 
*/
void AttributeDomainName::set(const std::string& value)
{
  string = value;
}



/* ****************************************************************************
*
* AttributeDomainName::get - 
*/
std::string AttributeDomainName::get(void)
{
  return string;
}



/* ****************************************************************************
*
* AttributeDomainName::render - 
*/
std::string AttributeDomainName::render(bool comma)
{
  if (string == "")
  {
    return "";
  }

  return valueTag("attributeDomainName", string, comma);
}



/* ****************************************************************************
*
* AttributeDomainName::c_str - 
*/
const char* AttributeDomainName::c_str(void)
{
  return string.c_str();
}



/* ****************************************************************************
*
* release - 
*/
void AttributeDomainName::release(void)
{
  /* This method is included for the sake of homogeneity */
}



/* ****************************************************************************
*
* fill - 
*/
void AttributeDomainName::fill(const AttributeDomainName& adn)
{
  string = adn.string;
}
