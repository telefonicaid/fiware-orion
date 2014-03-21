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
#include <string>

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
std::string ContextRegistration::render(Format format, std::string indent, bool comma, bool isInVector)
{
  std::string out = "";
  std::string tag = "contextRegistration";

  //
  // About JSON commas;
  // As providingApplication is MANDATORY and it is the last item in ContextRegistration,
  // the problem with the JSON commas disappear. All fields will have 'comma set to true'.
  // All, except providingApplication of course :-)
  //
  out += startTag(indent, tag, format, isInVector == false);
  out += entityIdVector.render(format, indent + "  ", true);
  out += contextRegistrationAttributeVector.render(format, indent + "  ", true);
  out += registrationMetadataVector.render(format, indent + "  ", true);
  out += providingApplication.render(format, indent + "  ", false);
  out += endTag(indent, tag, format, comma);

  return out;
}



/* ****************************************************************************
*
* ContextRegistration::check - 
*/
std::string ContextRegistration::check(RequestType requestType, Format format, std::string indent, std::string predetectedError, int counter)
{
  std::string res;

  if ((res = entityIdVector.check(requestType, format, indent, predetectedError, counter)) != "OK")                      return res;
  if ((res = contextRegistrationAttributeVector.check(requestType, format, indent, predetectedError, counter)) != "OK")  return res;
  if ((res = registrationMetadataVector.check(requestType, format, indent, predetectedError, counter)) != "OK")          return res;
  if ((res = providingApplication.check(requestType, format, indent, predetectedError, counter)) != "OK")                return res;

  if ((entityIdVectorPresent == true) && (entityIdVector.size() == 0))
    return "Empty entityIdVector";

  return "OK";
}



/* ****************************************************************************
*
* ContextRegistration::present - 
*/
void ContextRegistration::present(std::string indent, int ix)
{
  if (ix != -1)
    PRINTF("%sContext Registration %d:\n", indent.c_str(), ix);
  else
    PRINTF("%scontext registration:\n", indent.c_str());
      
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
