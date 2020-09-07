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
#include <string.h>   // strdup
#include <stdlib.h>   // free
#include <stdint.h>   // int64_t et al
#include <string>

#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"

#include "common/globals.h"
#include "common/tag.h"
#include "alarmMgr/alarmMgr.h"
#include "ngsi/Request.h"
#include "ngsi/Duration.h"



/* ****************************************************************************
*
* Duration::Duration
*
* Explicit constructor needed to initialize primitive types so they don't get
* random values from the stack
*/
Duration::Duration()
{
  seconds = 0;
  valid   = false;
  used    = false;
}



/* ****************************************************************************
*
* Duration::check -
*/
std::string Duration::check(void)
{
  if (string.empty())
  {
    return "OK";
  }

  if (parse() == -1)
  {
    valid = false;
    alarmMgr.badInput(clientIp, "syntax error in duration string");

    return "syntax error in duration string";
  }

  valid = true;

  return "OK";
}



/* ****************************************************************************
*
* Duration::set -
*/
void Duration::set(const std::string& value)
{
  string = value;
  parse();  // just to flag valid/invalid
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
  if (string.empty())
  {
    return true;
  }

  return false;
}



/* ****************************************************************************
*
* Duration::parse -
*/
int64_t Duration::parse(void)
{
  seconds = parse8601(string);

  valid = (seconds == -1)? false : true;

  return seconds;
}



/* ****************************************************************************
*
* Duration::toJsonV1 -
*/
std::string Duration::toJsonV1(bool comma)
{
  if (string.empty())
  {
    return "";
  }

  if (valid == false)
  {
    return "";
  }

  return valueTag("duration", string, comma);
}



/* ****************************************************************************
*
* release -
*/
void Duration::release(void)
{
  /* This method is included for the sake of homogeneity */
}
