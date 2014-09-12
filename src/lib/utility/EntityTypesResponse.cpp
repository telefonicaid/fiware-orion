/*
*
* Copyright 2014 Telefonica Investigacion y Desarrollo, S.A.U
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
#include <stdio.h>
#include <string>
#include <vector>

#include "common/Format.h"
#include "common/globals.h"
#include "common/tag.h"
#include "ngsi/Request.h"
#include "utility/EntityTypesResponse.h"



/* ****************************************************************************
*
* EntityTypesResponse::render - 
*/
std::string EntityTypesResponse::render(Format format, const std::string& indent)
{
  std::string out                 = "";
  std::string tag                 = "entityTypesResponse";
  std::string xmlEntityTypesTag   = "entityTypesFound";
  std::string jsonEntityTypesTag  = "typesFound";
  std::string xmlTypesVectorTag   = "entityTypes";
  std::string jsonTypesVectorTag  = "types";
  std::string noOfTypes;
  char        noOfTypesV[32];
  bool        commaAfterNoOfTypes = typeV.size() != 0;

  snprintf(noOfTypesV, sizeof(noOfTypesV), "%lu", typeV.size());
  noOfTypes = noOfTypesV;

  out += startTag(indent, tag, format, false);
  out += valueTag(indent + "  ", xmlEntityTypesTag, jsonEntityTypesTag, noOfTypes, format, commaAfterNoOfTypes);

  if (typeV.size() > 0)
  {
    out += startTag(indent + "  ", xmlTypesVectorTag, jsonTypesVectorTag, format, true, true);

    for (unsigned int ix = 0; ix < typeV.size(); ++ix)
    {
      out += valueTag(indent + "    ", "entityType", "", typeV[ix], format, (ix < typeV.size() - 1), false);
    }

    out += endTag(indent + "  ", xmlTypesVectorTag, format, false, true);
  }

  out += endTag(indent, tag, format, false);

  return out;
}



/* ****************************************************************************
*
* EntityTypesResponse::check - 
*/
std::string EntityTypesResponse::check
(
  RequestType         requestType,
  Format              format,
  const std::string&  indent,
  const std::string&  predetectedError,
  int                 counter
)
{
  if (predetectedError != "")
  {
    return predetectedError;
  }

  for (unsigned int ix = 0; ix < typeV.size(); ++ix)
  {
    if (typeV[ix] == "")
    {
      return "Empty type name";
    }
  }

  return "OK";
}



/* ****************************************************************************
*
* EntityTypesResponse::present - 
*/
void EntityTypesResponse::present(const std::string& indent)
{
  PRINTF("%s%lu types:\n", indent.c_str(), (unsigned long) typeV.size());

  for (unsigned int ix = 0; ix < typeV.size(); ++ix)
  {
    PRINTF("%s  %s\n", indent.c_str(), typeV[ix].c_str());
  }
}



/* ****************************************************************************
*
* EntityTypesResponse::push_back - 
*/
void EntityTypesResponse::push_back(const std::string& item)
{
  typeV.push_back(item);
}



/* ****************************************************************************
*
* EntityTypesResponse::size - 
*/
unsigned int EntityTypesResponse::size(void)
{
  return typeV.size();
}



/* ****************************************************************************
*
* EntityTypesResponse::release - 
*/
void EntityTypesResponse::release(void)
{
  typeV.clear();
}
