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
#include <string.h>
#include <string>

#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"

#include "common/globals.h"
#include "common/tag.h"
#include "ngsi/Request.h"
#include "ngsi/NotifyCondition.h"



/* ****************************************************************************
*
* NotifyCondition::NotifyCondition - 
*/
NotifyCondition::NotifyCondition()
{
}



/* ****************************************************************************
*
* NotifyCondition::NotifyCondition - 
*/
NotifyCondition::NotifyCondition(NotifyCondition* ncP)
{
  type = ncP->type;
  condValueList.fill(ncP->condValueList);
  restriction.set(ncP->restriction.get());
}



/* ****************************************************************************
*
* NotifyCondition::render -
*/
std::string NotifyCondition::render(const std::string& indent, bool notLastInVector)
{
  std::string out = "";

  bool condValueListRendered   = condValueList.size() != 0;
  bool restrictionRendered     = restriction.get() != "";
  bool commaAfterRestriction   = false;  // last element
  bool commaAfterCondValueList = restrictionRendered;
  bool commaAfterType          = condValueListRendered || restrictionRendered;

  out += startTag(indent);
  out += valueTag(indent + "  ", "type", type, commaAfterType);
  out += condValueList.render(indent + "  ",   commaAfterCondValueList);
  out += restriction.render(  indent + "  ",   commaAfterRestriction);
  out += endTag(indent);

  return out;
}



/* ****************************************************************************
*
* NotifyCondition::check -
*
* FIXME: P5 - in case of errors a formated result string should be returned ... ?
*/
std::string NotifyCondition::check
(
  RequestType         requestType,
  const std::string&  indent,
  const std::string&  predetectedError,
  int                 counter
)
{
  std::string res;

  if (type == "")
  {
    return "empty type for NotifyCondition";
  }
  else if (strcasecmp(type.c_str(), ON_CHANGE_CONDITION) == 0)
  {
  }
  else
  {
    return std::string("invalid notify condition type: /") + type + "/";
  }

  if ((res = condValueList.check(requestType, indent, predetectedError, counter)) != "OK")
  {
    return res;
  }

  if ((res = restriction.check(requestType, indent, predetectedError, counter)) != "OK")
  {
    return res;
  }

  return "OK";
}



/* ****************************************************************************
*
* NotifyCondition::present -
*/
void NotifyCondition::present(const std::string& indent, int ix)
{
  std::string indent2 = indent + "  ";

  if (ix == -1)
  {
    LM_T(LmtPresent, ("%sNotify Condition:", 
		      indent2.c_str()));
  }
  else
  {
    LM_T(LmtPresent, ("%sNotify Condition %d:", 
		      indent2.c_str(), 
		      ix));
  }

  LM_T(LmtPresent, ("%stype: %s", 
		    indent2.c_str(), 
		    type.c_str()));
  condValueList.present(indent2);
  restriction.present(indent2);
}



/* ****************************************************************************
*
* NotifyCondition::release -
*/
void NotifyCondition::release(void)
{
  condValueList.release();
}
