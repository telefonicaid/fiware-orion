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
#include "common/idCheck.h"
#include "common/tag.h"
#include "ngsi/Request.h"
#include "ngsi/RegistrationId.h"



/* ****************************************************************************
*
* RegistrationId::RegistrationId -
*/
RegistrationId::RegistrationId()
{
}



/* ****************************************************************************
*
* RegistrationId::RegistrationId -
*/
RegistrationId::RegistrationId(const std::string& regId) : string(regId)
{
}



/* ****************************************************************************
*
* RegistrationId::check -
*/
std::string RegistrationId::check(void)
{
  std::string out = "OK";

  if (!string.empty())
  {
    out = idCheck(string);
  }

  return out;
}



/* ****************************************************************************
*
* RegistrationId::isEmpty -
*/
bool RegistrationId::isEmpty(void)
{
  return (string.empty())? true : false;
}



/* ****************************************************************************
*
* RegistrationId::set -
*/
void RegistrationId::set(const std::string& value)
{
  string = value;
}



/* ****************************************************************************
*
* RegistrationId::get -
*/
std::string RegistrationId::get(void) const
{
  return string;
}



/* ****************************************************************************
*
* RegistrationId::toJsonV1 -
*/
std::string RegistrationId::toJsonV1(RequestType requestType, bool comma)
{
  if (string.empty())
  {
    if (requestType == RegisterResponse)  // registrationId is MANDATORY for RegisterContextResponse
    {
      string = "000000000000000000000000";
      LM_T(LmtOldInfo, ("No registrationId - setting the registrationId to 24 zeroes"));
    }
    else
    {
      return "";
    }
  }

  return valueTag("registrationId", string, comma);
}



/* ****************************************************************************
*
* release -
*/
void RegistrationId::release(void)
{
  /* This method is included for the sake of homogeneity */
}
