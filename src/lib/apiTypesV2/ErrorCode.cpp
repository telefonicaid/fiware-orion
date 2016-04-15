/*
*
* Copyright 2015 Telefonica Investigacion y Desarrollo, S.A.U
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
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <string>

#include "logMsg/logMsg.h"

#include "apiTypesV2/ErrorCode.h"
#include "common/string.h"
#include "common/tag.h"



/* ****************************************************************************
*
* ErrorCode::ErrorCode -
*/
ErrorCode::ErrorCode()
{
  error        = "OK";
  description  = "";
}



/* ****************************************************************************
*
* ErrorCode::ErrorCode -
*/
ErrorCode::ErrorCode(const std::string& _error, const std::string& _description)
{
  error        = _error;
  description  = _description;
}



/* ****************************************************************************
*
* toJson - 
*/
std::string ErrorCode::toJson(bool isLastElement)
{
  std::string  out  = "";

  if ((description == "") && ((error == "") || (error == "OK")))
  {
    return "";
  }

  if (strstr(description.c_str(), "\"") != NULL)
  {
    int    len = description.length() * 2;
    char*  s    = (char*) calloc(1, len + 1);

    strReplace(s, len, description.c_str(), "\"", "\\\"");
    description = s;
    free(s);
  }

  out += "{";

  bool comma = false;
  if ((error != "") && (error != "OK"))
  {
    out += JSON_VALUE("error", error);
    comma = true;
  }

  if (description != "")
  {
    if (comma == true)
    {
      out += ",";
    }

    out += JSON_VALUE("description", description);
  }

  out += "}";

  if (isLastElement == false)
  {
    out += ",";
  }

  return out;
}



/* ****************************************************************************
*
* fill - 
*/
void ErrorCode::fill(const std::string& _error, const std::string& _description)
{
  error        = _error;
  description  = _description;
}



/* ****************************************************************************
*
* fill - 
*/
void ErrorCode::fill(const StatusCode& sc)
{
  error       = sc.reasonPhrase;
  description = sc.details;
}
