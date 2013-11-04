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
#include "common/idCheck.h"
#include "common/tag.h"
#include "ngsi/Request.h"
#include "ngsi/SubscriptionId.h"



/* ****************************************************************************
*
* SubscriptionId::check - 
*/
std::string SubscriptionId::check(RequestType requestType, Format format, std::string indent, std::string predetectedError, int counter)
{
  std::string out = "OK";

  if (string != "")
    out = idCheck(string);

  return out;
}



/* ****************************************************************************
*
* SubscriptionId::isEmpty - 
*/
bool SubscriptionId::isEmpty(void)
{
   return (string == "")? true : false;
}



/* ****************************************************************************
*
* SubscriptionId::set - 
*/
void SubscriptionId::set(std::string value)
{
  string = value;
}



/* ****************************************************************************
*
* SubscriptionId::get - 
*/
std::string SubscriptionId::get(void)
{
  return string;
}



/* ****************************************************************************
*
* SubscriptionId::present - 
*/
void SubscriptionId::present(std::string indent)
{
  if (string != "")
    PRINTF("%sSubscriptionId: %s\n", indent.c_str(), string.c_str());
  else
    PRINTF("%sNo SubscriptionId\n", indent.c_str());
}



/* ****************************************************************************
*
* SubscriptionId::render - 
*/
std::string SubscriptionId::render(Format format, std::string indent, bool comma)
{
  std::string xString = string;
  
  if (xString == "")
    xString = std::string("No Subscription ID");

  return valueTag(indent, "subscriptionId", xString, format, comma);
}



/* ****************************************************************************
*
* release - 
*/
void SubscriptionId::release(void)
{
   /* This method is included for the sake of homogeneity */
   string = "";
}
