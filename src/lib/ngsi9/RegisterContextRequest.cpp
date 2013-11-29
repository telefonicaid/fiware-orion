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
#include <string>

#include "common/globals.h"
#include "common/tag.h"
#include "common/Format.h"
#include "ngsi/StatusCode.h"
#include "ngsi/Duration.h"
#include "ngsi/ContextRegistrationVector.h"
#include "ngsi9/RegisterContextResponse.h"
#include "ngsi9/RegisterContextRequest.h"



/* ****************************************************************************
*
* RegisterContextRequest::render - 
*/
std::string RegisterContextRequest::render(RequestType requestType, Format format, std::string indent)
{
  std::string out = "";
  std::string tag = "registerContextRequest";

  out += startTag(indent, tag, format, false);
  out += contextRegistrationVector.render(format, indent + "  ");
  out += duration.render(format, indent + "  ");
  out += registrationId.render(format, indent + "  ");
  out += endTag(indent, tag, format);

  return out;
}



/* ****************************************************************************
*
* RegisterContextRequest::check - 
*/
std::string RegisterContextRequest::check(RequestType requestType, Format format, std::string indent, std::string predetectedError, int counter)
{
  RegisterContextResponse  response(this);
  std::string              res;

  if (predetectedError != "")
  {
    LM_E(("predetectedError not empty"));
    response.errorCode.code         = SccBadRequest;
    response.errorCode.reasonPhrase = predetectedError;
  }
  else if (contextRegistrationVector.size() == 0)
  {
    LM_E(("contextRegistrationVector.size() == 0"));
    response.errorCode.code         = SccBadRequest;
    response.errorCode.reasonPhrase = "Empty Context Registration List";
  }
  else if (((res = contextRegistrationVector.check(RegisterContext, format, indent, predetectedError, counter)) != "OK") ||
           ((res = duration.check(RegisterContext, format, indent, predetectedError, counter))                  != "OK") ||
           ((res = registrationId.check(RegisterContext, format, indent, predetectedError, counter))            != "OK"))
  {
    LM_E(("Some check method failed"));
    response.errorCode.code         = SccBadRequest;
    response.errorCode.reasonPhrase = res;
  }
  else
    return "OK";

  LM_E(("Not OK - returning rendered error result"));     
  return response.render(RegisterContext, format, indent);
}



/* ****************************************************************************
*
* RegisterContextRequest::release - 
*/
void RegisterContextRequest::release(void)
{
  contextRegistrationVector.release();
  duration.release();
  registrationId.release();
}



/* ****************************************************************************
*
* RegisterContextRequest::fill - 
*/
void RegisterContextRequest::fill(RegisterProviderRequest& rpr, std::string entityId, std::string entityType, std::string attributeName)
{
  ContextRegistration*          crP        = new ContextRegistration();
  EntityId*                     entityIdP  = new EntityId(entityId, entityType, "false");

  duration       = rpr.duration;
  registrationId = rpr.registrationId;

  crP->registrationMetadataVector.fill(rpr.metadataVector);
  crP->providingApplication = rpr.providingApplication;

  crP->entityIdVector.push_back(entityIdP);
  crP->entityIdVectorPresent = true;

  if (attributeName != "")
  {
    ContextRegistrationAttribute* attributeP = new ContextRegistrationAttribute(attributeName, "", "false");

    crP->contextRegistrationAttributeVector.push_back(attributeP);
  }

  contextRegistrationVector.push_back(crP);
}
