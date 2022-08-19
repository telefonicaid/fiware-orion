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
#include "ngsi/AttributeExpression.h"


/* ****************************************************************************
*
* AttributeExpression::check - 
*/
std::string AttributeExpression::check(void)
{
  return "OK";
}



/* ****************************************************************************
*
* AttributeExpression::isEmpty - 
*/
bool AttributeExpression::isEmpty(void)
{
  if (string.empty())
  {
    return true;
  }

  return false;
}



/* ****************************************************************************
*
* AttributeExpression::set - 
*/
void AttributeExpression::set(const std::string& value)
{
  string = value;
}



/* ****************************************************************************
*
* AttributeExpression::get - 
*/
std::string AttributeExpression::get(void)
{
  return string;
}



/* ****************************************************************************
*
* AttributeExpression::toJsonV1 -
*/
std::string AttributeExpression::toJsonV1(bool comma)
{
  if (string.empty())
  {
    return "";
  }

  return valueTag("attributeExpression", string, comma);
}



/* ****************************************************************************
*
* AttributeExpression::c_str - 
*/
const char* AttributeExpression::c_str(void)
{
  return string.c_str();
}



/* ****************************************************************************
*
* AttributeExpression::release - 
*/
void AttributeExpression::release(void)
{
  /* This method is included for the sake of homogeneity */
}
