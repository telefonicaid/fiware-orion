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
#include "ngsi/Request.h"
#include "ngsi/ContextRegistration.h"
#include "ngsi/ContextRegistrationAttribute.h"
#include "ngsi/EntityId.h"

/* ****************************************************************************
*
* ContextRegistration::ContextRegistration -
*
* Explicit constructor needed to initialize primitive types so they don't get
* random values from the stack
*/
ContextRegistration::ContextRegistration()
{
  entityIdVectorPresent = false;
}

/* ****************************************************************************
*
* ContextRegistration::render -
*/
std::string ContextRegistration::render(bool comma, bool isInVector)
{
  std::string out = "";

  //
  // About JSON commas;
  // As providingApplication is MANDATORY and it is the last item in ContextRegistration,
  // the problem with the JSON commas disappear. All fields will have 'comma set to true'.
  // All, except providingApplication of course :-)
  //

  out += startTag(!isInVector? "contextRegistration" : "");
  out += entityIdVector.render(true);
  out += contextRegistrationAttributeVector.render(true);
  out += registrationMetadataVector.render(true);
  out += providingApplication.render(false);
  out += endTag(comma);

  return out;
}



/* ****************************************************************************
*
* ContextRegistration::check -
*/
std::string ContextRegistration::check
(
  ApiVersion          apiVersion,
  RequestType         requestType,
  const std::string&  predetectedError,
  int                 counter
)
{
  std::string res;

  if ((res = entityIdVector.check(requestType)) != "OK")
  {
    return res;
  }

  if ((res = contextRegistrationAttributeVector.check(apiVersion)) != "OK")
  {
    return res;
  }

  if ((res = registrationMetadataVector.check(apiVersion)) != "OK")
  {
    return res;
  }

  if ((res = providingApplication.check()) != "OK")
  {
    return res;
  }

  if ((entityIdVectorPresent == true) && (entityIdVector.size() == 0))
  {
    return "Empty entityIdVector";
  }

  return "OK";
}



/* ****************************************************************************
*
* ContextRegistration::present -
*/
void ContextRegistration::present(const std::string& indent, int ix)
{
  if (ix != -1)
  {
    LM_T(LmtPresent, ("%sContext Registration %d:\n", 
		      indent.c_str(), 
		      ix));
  }
  else
  {
    LM_T(LmtPresent, ("%scontext registration:\n", indent.c_str()));
  }

  entityIdVector.present(indent + "  ");
  contextRegistrationAttributeVector.present(indent + "  ");
  registrationMetadataVector.present("Registration", indent + "  ");
  providingApplication.present(indent + "  ");
}



/* ****************************************************************************
*
* ContextRegistration::release -
*/
void ContextRegistration::release(void)
{
  entityIdVector.release();
  contextRegistrationAttributeVector.release();
  registrationMetadataVector.release();
  providingApplication.release();
}
