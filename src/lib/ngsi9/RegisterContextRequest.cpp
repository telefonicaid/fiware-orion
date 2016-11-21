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
#include <string>

#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"

#include "common/globals.h"
#include "common/tag.h"
#include "alarmMgr/alarmMgr.h"
#include "ngsi/StatusCode.h"
#include "ngsi/Duration.h"
#include "ngsi/ContextRegistrationVector.h"
#include "ngsi9/RegisterContextResponse.h"
#include "ngsi9/RegisterContextRequest.h"



/* ****************************************************************************
*
* RegisterContextRequest::render - 
*/
std::string RegisterContextRequest::render(const std::string& indent)
{
  std::string  out                                 = "";
  bool         durationRendered                    = duration.get() != "";
  bool         registrationIdRendered              = registrationId.get() != "";
  bool         commaAfterRegistrationId            = false; // Last element
  bool         commaAfterDuration                  = registrationIdRendered;
  bool         commaAfterContextRegistrationVector = registrationIdRendered || durationRendered;

  out += startTag(indent);

  out += contextRegistrationVector.render(      indent + "  ", commaAfterContextRegistrationVector);
  out += duration.render(                       indent + "  ", commaAfterDuration);
  out += registrationId.render(RegisterContext, indent + "  ", commaAfterRegistrationId);

  out += endTag(indent, false);

  return out;
}



/* ****************************************************************************
*
* RegisterContextRequest::check - 
*/
std::string RegisterContextRequest::check(const std::string& apiVersion, const std::string& indent, const std::string& predetectedError, int counter)
{
  RegisterContextResponse  response(this);
  std::string              res;

  if (predetectedError != "")
  {
    alarmMgr.badInput(clientIp, predetectedError);
    response.errorCode.fill(SccBadRequest, predetectedError);
  }
  else if (contextRegistrationVector.size() == 0)
  {
    alarmMgr.badInput(clientIp, "empty contextRegistration list");
    response.errorCode.fill(SccBadRequest, "Empty Context Registration List");
  } 
  else if (((res = contextRegistrationVector.check(apiVersion, RegisterContext, indent, predetectedError, counter)) != "OK") ||
           ((res = duration.check(RegisterContext, indent, predetectedError, counter))                  != "OK") ||
           ((res = registrationId.check(RegisterContext, indent, predetectedError, counter))            != "OK"))
  {
    alarmMgr.badInput(clientIp, res);
    response.errorCode.fill(SccBadRequest, res);
  }
  else
  {
    return "OK";
  }

  return response.render(indent);
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
void RegisterContextRequest::fill(RegisterProviderRequest& rpr, const std::string& entityId, const std::string& entityType, const std::string& attributeName)
{
  ContextRegistration*          crP        = new ContextRegistration();
  EntityId*                     entityIdP  = new EntityId(entityId, entityType, "false");

  duration       = rpr.duration;
  registrationId = rpr.registrationId;

  crP->registrationMetadataVector.fill((MetadataVector*) &rpr.metadataVector);
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
