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
#include <stdio.h>

#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"

#include "common/globals.h"
#include "common/tag.h"
#include "ngsi/Metadata.h"



/* ****************************************************************************
*
* Metadata::Metadata - 
*/ 
Metadata::Metadata()
{
  name  = "";
  type  = "";
  value = "";
}



/* ****************************************************************************
*
* Metadata::Metadata - 
*
* FIXME P9: Copy also the Association!
*  
*/ 
Metadata::Metadata(Metadata* mP)
{
  LM_T(LmtClone, ("'cloning' a Metadata"));

  name  = mP->name;
  type  = mP->type;
  value = mP->value;
}



/* ****************************************************************************
*
* Metadata::Metadata - 
*/ 
Metadata::Metadata(const std::string& _name, const std::string& _type, const std::string& _value)
{
  name  = _name;
  type  = _type;
  value = _value;
}



/* ****************************************************************************
*
* Metadata::render - 
*/
std::string Metadata::render(Format format, const std::string& indent, bool comma)
{
  std::string out     = "";
  std::string tag  = "contextMetadata";
  std::string xValue  = value;

  out += startTag(indent, tag, tag, format, false, false);
  out += valueTag(indent + "  ", "name", name, format, true);
  out += valueTag(indent + "  ", "type", type, format, true);

  if (type == "Association")
    xValue = std::string("\n") + association.render(format, indent + "  ", false);

  out += valueTag(indent + "  ", "value", xValue, format, false, (type == "Association"));
  out += endTag(indent, tag, format, comma);

  return out;
}



/* ****************************************************************************
*
* Metadata::check - 
*/
std::string Metadata::check(RequestType requestType, Format format, const std::string& indent, const std::string& predetectedError, int counter)
{
  if (name == "")
    return "missing metadata name";

  if ((value == "") && (type != "Association"))
    return "missing metadata value";

  if (type == "Association")
     return association.check(requestType, format, indent, predetectedError, counter);

  return "OK";
}



/* ****************************************************************************
*
* Metadata::present - 
*/
void Metadata::present(const std::string& metadataType, int ix, const std::string& indent)
{
  PRINTF("%s%s Metadata %d:\n",   indent.c_str(), metadataType.c_str(), ix);
  PRINTF("%s  Name:     %s\n", indent.c_str(), name.c_str());
  PRINTF("%s  Type:     %s\n", indent.c_str(), type.c_str());
  PRINTF("%s  Value:    %s\n", indent.c_str(), value.c_str());
}



/* ****************************************************************************
*
* release - 
*/
void Metadata::release(void)
{
   association.release();
}
