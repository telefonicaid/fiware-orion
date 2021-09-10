/*
*
* Copyright 2021 Telefonica Investigacion y Desarrollo, S.A.U
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
#include "ngsi/FailsCounter.h"


/* ****************************************************************************
*
* FailsCounter::parse -
*/
int64_t FailsCounter::parse(void)
{
  seconds = parse8601(string);
  return seconds;
}



/* ****************************************************************************
*
* FailsCounter::check -
*/
std::string FailsCounter::check(void)
{
  if (string.empty())
  {
    return "OK";
  }

  if (parse() == -1)
  {
    return "syntax error in failsCounter string";
  }

  return "OK";
}



/* ****************************************************************************
*
* FailsCounter::isEmpty -
*/
bool FailsCounter::isEmpty(void)
{
  return (string.empty())? true : false;
}



/* ****************************************************************************
*
* FailsCounter::set -
*/
void FailsCounter::set(const std::string& value)
{
  string = value;
}



/* ****************************************************************************
*
* FailsCounter::get -
*/
const std::string FailsCounter::get(void)
{
  return string;
}



/* ****************************************************************************
*
* FailsCounter::toJsonV1 -
*/
std::string FailsCounter::toJsonV1(bool comma)
{
  if (string.empty())
  {
    return "";
  }

  return valueTag("FailsCounter", string, comma);
}
