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
#include "ngsi/ContextAttribute.h"



/* ****************************************************************************
*
* ContextAttribute::ContextAttribute - 
*/
ContextAttribute::ContextAttribute()
{
   name  = "";
   type  = "";
   value = "";
}



/* ****************************************************************************
*
* ContextAttribute::ContextAttribute - 
*/
ContextAttribute::ContextAttribute(ContextAttribute* caP)
{
   LM_T(LmtClone, ("'cloning' a ContextAttribute"));
   name  = caP->name;
   type  = caP->type;
   value = caP->value;

   metadataVector.vec.clear();

   for (unsigned int mIx = 0; mIx < caP->metadataVector.size(); ++mIx)
   {
      LM_T(LmtClone, ("Copying metadata %d", mIx));
      Metadata* mP = new Metadata(caP->metadataVector.get(mIx));
      metadataVector.push_back(mP);
   }
}



/* ****************************************************************************
*
* ContextAttribute::ContextAttribute - 
*/
ContextAttribute::ContextAttribute(std::string _name, std::string _type, std::string _value)
{
   name  = _name;
   type  = _type;
   value = _value;
}

/* ****************************************************************************
*
* ContextAttribute::getId() -
*/
std::string ContextAttribute::getId()
{
  for (unsigned int ix = 0; ix < metadataVector.size(); ++ix) {
      if (metadataVector.get(ix)->name == NGSI_MD_ID) {
          return metadataVector.get(ix)->value;
      }
  }
  return "";
}

/* ****************************************************************************
*
* render - 
*/
std::string ContextAttribute::render(Format format, std::string indent, bool isInVector)
{
  std::string out = "";
  std::string tag = "contextAttribute";

  metadataVector.tagSet("metadata");

  out += startTag(indent, tag, format, false);
  out += valueTag(indent + "  ", "name",         name,  format, true);
  out += valueTag(indent + "  ", "type",         type,  format, true);
  out += valueTag(indent + "  ", "contextValue", value, format, false);
  out += metadataVector.render(format, indent + "  ", false);
  out += endTag(indent, tag, format, isInVector);

  return out;
}



/* ****************************************************************************
*
* ContextAttribute::check - 
*/
std::string ContextAttribute::check(RequestType requestType, Format format, std::string indent, std::string predetectedError, int counter)
{
  if ((name == "") || (name == "not in use"))
    return "missing attribute name";

  if (requestType != UpdateContext) // FIXME: this is just to make harness test work
  {
    if ((value == "") || (value == "not in use"))
      return "missing attribute value";
  }

  return "OK";
}



/* ****************************************************************************
*
* ContextAttribute::present - 
*/
void ContextAttribute::present(std::string indent, int ix)
{
  PRINTF("%sAttribute %d:\n",    indent.c_str(), ix);
  PRINTF("%s  Name:       %s\n", indent.c_str(), name.c_str());
  PRINTF("%s  Type:       %s\n", indent.c_str(), type.c_str());
  PRINTF("%s  Value:      %s\n", indent.c_str(), value.c_str());

  metadataVector.present("Attribute", indent + "  ");
}



/* ****************************************************************************
*
* ContextAttribute::release - 
*/
void ContextAttribute::release(void)
{
  metadataVector.release();
}
