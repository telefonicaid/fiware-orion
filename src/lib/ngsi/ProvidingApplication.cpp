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
#include "ngsi/ProvidingApplication.h"



/* ****************************************************************************
*
* ProvidingApplication::ProvidingApplication -
*/
ProvidingApplication::ProvidingApplication()
{
  /* It is better to have a default constructor than to leave mimeType with a random value */
  string         = "";
  providerFormat = PfJson;
}



/* ****************************************************************************
*
* ProvidingApplication::check -
*/
std::string ProvidingApplication::check(void)
{
  if (isEmpty())
  {
    return "no providing application";
  }

  return "OK";
}



/* ****************************************************************************
*
* ProvidingApplication::isEmpty -
*/
bool ProvidingApplication::isEmpty(void)
{
  return (string.empty())? true : false;
}



/* ****************************************************************************
*
* ProvidingApplication::set -
*/
void ProvidingApplication::set(const std::string& value)
{
  string = value;
}



/* ****************************************************************************
*
* ProvidingApplication::setProviderFormat -
*/
void ProvidingApplication::setProviderFormat(const ProviderFormat _providerFormat)
{
  providerFormat = _providerFormat;
}



/* ****************************************************************************
*
* ProvidingApplication::setRegId -
*/
void ProvidingApplication::setRegId(const std::string& _regId)
{
  regId = _regId;
}



/* ****************************************************************************
*
* ProvidingApplication::getProviderFormat -
*/
ProviderFormat ProvidingApplication::getProviderFormat(void)
{
  return providerFormat;
}



/* ****************************************************************************
*
* ProvidingApplication::getRegId -
*/
std::string ProvidingApplication::getRegId(void)
{
  return regId;
}



/* ****************************************************************************
*
* ProvidingApplication::getRegId -
*/
std::string ProvidingApplication::get(void)
{
  return string;
}



/* ****************************************************************************
*
* ProvidingApplication::toJsonV1 -
*/
std::string ProvidingApplication::toJsonV1(bool comma)
{
  if (string.empty())
  {
    return "";
  }

  return valueTag("providingApplication", string, comma);
}



/* ****************************************************************************
*
* ProvidingApplication::c_str -
*/
const char* ProvidingApplication::c_str(void)
{
  return string.c_str();
}



/* ****************************************************************************
*
* release -
*/
void ProvidingApplication::release(void)
{
  /* This method is included for the sake of homogeneity */
}
