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
#include <stdio.h>
#include <string>

#include "logMsg/logMsg.h"

#include "common/globals.h"
#include "common/tag.h"
#include "ngsi/AttributeAssociation.h"
#include "ngsi/Request.h"



/* ****************************************************************************
*
* render - 
*/
std::string AttributeAssociation::render(Format format, const std::string& indent, bool comma)
{
  std::string  out                     = "";
  std::string  tag                     = "attributeAssociation";
  std::string  srcXmlTag               = "sourceAttribute";
  std::string  tgtXmlTag               = "targetAttribute";
  std::string  srcJsonTag              = "source";
  std::string  tgtJsonTag              = "target";

  bool         targetAttributeRendered = target != "";

  out += startTag(indent, tag, format, false);
  out += valueTag(indent + "  ", (format == XML? srcXmlTag : srcJsonTag), source, format, targetAttributeRendered);
  out += valueTag(indent + "  ", (format == XML? tgtXmlTag : tgtJsonTag), target, format, false);
  out += endTag(indent, tag, format, comma);

  return out;
}



/* ****************************************************************************
*
* check - 
*/
std::string AttributeAssociation::check
(
  RequestType         requestType,
  Format              format,
  const std::string&  indent,
  const std::string&  predetectedError,
  int                 counter
)
{
  if (source == "")
  {
    return "empty source";
  }

  if (target == "")
  {
    return "empty target";
  }

  return "OK";
}



/* ****************************************************************************
*
* present - 
*/
void AttributeAssociation::present(const std::string& indent, int ix)
{
  if (ix == -1)
  {
    LM_F(("%sAttribute Association:\n",indent.c_str()));
  }
else
  {
    LM_F(("%sAttribute Association %d:\n",indent.c_str(), ix));
  }

  LM_F(("%s  Source: %s\n", indent.c_str(), source.c_str()));
  LM_F(("%s  Target: %s\n\n", indent.c_str(), target.c_str()));
}
