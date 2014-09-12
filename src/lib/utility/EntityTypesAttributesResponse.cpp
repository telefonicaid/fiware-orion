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
#include "utility/EntityTypesAttributesResponse.h"



/* ****************************************************************************
*
* EntityTypesAttributesResponse::render - 
*/
std::string EntityTypesAttributesResponse::render(Format format, const std::string& indent)
{
  std::string out                           = "";
  std::string tag                           = "entityTypeAttributesResponse";
  std::string xmlEntityTypesAttributesTag   = "attributesFound";
  std::string jsonEntityTypesAttributesTag  = "attributesFound";
  std::string xmlTypesVectorTag             = "attributes";
  std::string jsonTypesVectorTag            = "attributes";
  std::string noOfAttributes;
  char        noOfAttributesV[32];
  bool        commaAfterNoOfTypes           = attributeV.size() != 0;

  snprintf(noOfAttributesV, sizeof(noOfAttributesV), "%lu", attributeV.size());
  noOfAttributes = noOfAttributesV;

  out += startTag(indent, tag, format, false);
  out += valueTag(indent + "  ", xmlEntityTypesAttributesTag, jsonEntityTypesAttributesTag, noOfAttributes, format, commaAfterNoOfTypes);

  if (attributeV.size() > 0)
  {
    out += startTag(indent + "  ", xmlTypesVectorTag, jsonTypesVectorTag, format, true, true);

    for (unsigned int ix = 0; ix < attributeV.size(); ++ix)
    {
      out += valueTag(indent + "    ", "attribute", "", attributeV[ix], format, (ix < attributeV.size() - 1), false);
    }

    out += endTag(indent + "  ", xmlTypesVectorTag, format, false, true);
  }

  out += endTag(indent, tag, format, false);

  return out;
}



/* ****************************************************************************
*
* EntityTypesAttributesResponse::check - 
*/
std::string EntityTypesAttributesResponse::check
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

  for (unsigned int ix = 0; ix < attributeV.size(); ++ix)
  {
    if (attributeV[ix] == "")
    {
      return "Empty type name";
    }
  }

  return "OK";
}



/* ****************************************************************************
*
* EntityTypesAttributesResponse::present - 
*/
void EntityTypesAttributesResponse::present(const std::string& indent)
{
  PRINTF("%s%lu types:\n", indent.c_str(), (unsigned long) attributeV.size());

  for (unsigned int ix = 0; ix < attributeV.size(); ++ix)
  {
    PRINTF("%s  %s\n", indent.c_str(), attributeV[ix].c_str());
  }
}



/* ****************************************************************************
*
* EntityTypesAttributesResponse::push_back - 
*/
void EntityTypesAttributesResponse::push_back(const std::string& item)
{
  attributeV.push_back(item);
}



/* ****************************************************************************
*
* EntityTypesAttributesResponse::size - 
*/
unsigned int EntityTypesAttributesResponse::size(void)
{
  return attributeV.size();
}



/* ****************************************************************************
*
* EntityTypesAttributesResponse::release - 
*/
void EntityTypesAttributesResponse::release(void)
{
  attributeV.clear();
}
