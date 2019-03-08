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
* Author: Fermin Galan
*/
#include <string>

#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"

#include "common/globals.h"
#include "common/tag.h"
#include "ngsi/Request.h"
#include "ngsi/Originator.h"



/* ****************************************************************************
*
* Originator::check -
*/
std::string Originator::check(void)
{
  return "OK";
}



/* ****************************************************************************
*
* Originator::isEmpty -
*/
bool Originator::isEmpty(void)
{
  return (string == "")? true : false;
}



/* ****************************************************************************
*
* Originator::set -
*/
void Originator::set(const std::string& value)
{
  string = value;
}



/* ****************************************************************************
*
* Originator::get -
*/
std::string Originator::get(void)
{
  return string;
}



/* ****************************************************************************
*
* Originator::toJsonV1 -
*/
std::string Originator::toJsonV1(bool comma)
{
  if (string == "")
  {
    return "";
  }

  return valueTag("originator", string, comma);
}



/* ****************************************************************************
*
* Originator::c_str -
*/
const char* Originator::c_str(void)
{
  return string.c_str();
}
