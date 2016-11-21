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
#include "logMsg/traceLevels.h"

#include "common/globals.h"
#include "common/tag.h"
#include "ngsi/ContextRegistrationAttribute.h"



/* ****************************************************************************
*
* ContextRegistrationAttribute::ContextRegistrationAttribute -
*/
ContextRegistrationAttribute::ContextRegistrationAttribute()
{
  name     = "";
  type     = "";
  isDomain = "";
}



/* ****************************************************************************
*
* ContextRegistrationAttribute::ContextRegistrationAttribute -
*/
ContextRegistrationAttribute::ContextRegistrationAttribute
(
  const std::string&  _name,
  const std::string&  _type,
  const std::string&  _isDomain
)
{
  name      = _name;
  type      = _type;
  isDomain  = _isDomain;
}

/* ****************************************************************************
*
* ContextRegistrationAttribute::render -
*/
std::string ContextRegistrationAttribute::render(const std::string& indent, bool comma)
{
  std::string out = "";

  //
  // About JSON commas:
  // The field isDomain is mandatory, so all field before that will
  // have the comma set to true for the render methods.
  // The only doubt here is whether isDomain should have the comma or not,
  // that depends on whether the metadataVector is empty or not.
  //
  out += startTag(indent);
  out += valueTag(indent + "  ", "name",     name, true);
  out += valueTag(indent + "  ", "type",     type, true);
  out += valueTag(indent + "  ", "isDomain", isDomain, metadataVector.size() != 0);
  out += metadataVector.render(indent + "  ");
  out += endTag(indent, comma);

  return out;
}



/* ****************************************************************************
*
* ContextRegistrationAttribute::check -
*/
std::string ContextRegistrationAttribute::check
(
  const std::string&  apiVersion,
  const std::string&  indent
)
{

  if (name == "")
  {
    return "missing name for registration attribute";
  }

  if (isDomain == "")
  {
    return "missing isDomain value for registration attribute";
  }

  if (!isTrue(isDomain) && !isFalse(isDomain))
  {
    return std::string("invalid isDomain value for registration attribute: /") + isDomain + "/";
  }

  std::string res;
  if ((res = metadataVector.check(apiVersion)) != "OK")
  {
    return res;
  }

  return "OK";
}



/* ****************************************************************************
*
* ContextRegistrationAttribute::present -
*/
void ContextRegistrationAttribute::present(int ix, const std::string& indent)
{
  LM_T(LmtPresent, ("%sAttribute %d:\n",    
		    indent.c_str(), 
		    ix));
  LM_T(LmtPresent, ("%s  Name:       %s\n", 
		    indent.c_str(), 
		    name.c_str()));
  LM_T(LmtPresent, ("%s  Type:       %s\n", 
		    indent.c_str(), 
		    type.c_str()));
  LM_T(LmtPresent, ("%s  isDomain:   %s\n", 
		    indent.c_str(), 
		    isDomain.c_str()));

  metadataVector.present("Attribute", indent + "  ");
}



/* ****************************************************************************
*
* ContextRegistrationAttribute::release -
*/
void ContextRegistrationAttribute::release(void)
{
  metadataVector.release();
}
