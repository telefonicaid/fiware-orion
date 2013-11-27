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
#include <vector>

#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"

#include "ngsi/ParseData.h"
#include "ngsi/EntityId.h"
#include "ngsi/ContextRegistration.h"
#include "ngsi/ContextRegistrationAttribute.h"
#include "ngsi9/RegisterContextResponse.h"
#include "rest/ConnectionInfo.h"
#include "serviceRoutines/postRegisterContext.h"  // instead of convenienceMap function, postRegisterContext is used
#include "serviceRoutines/postContextEntityTypeAttribute.h"



/* ****************************************************************************
*
* postContextEntityTypeAttribute - 
*/
std::string postContextEntityTypeAttribute(ConnectionInfo* ciP, int components, std::vector<std::string> compV, ParseData* parseDataP)
{
  std::string  typeName     = compV[2];
  std::string  attributeName  = compV[4];  
  std::string  answer;
  
  // Transform RegisterProviderRequest into RegisterContextRequest
  RegisterProviderRequest*      rprP = &parseDataP->rpr.res;
  RegisterContextRequest*       rcrP = &parseDataP->rcr.res;
  ContextRegistration           cr;
  EntityId                      entityId;
  ContextRegistrationAttribute  attribute(attributeName, "", "false");

  rcrP->duration       = rprP->duration;
  rcrP->registrationId = rprP->registrationId;

  entityId.type = typeName;
  cr.registrationMetadataVector.fill(rprP->metadataVector);
  cr.providingApplication = rprP->providingApplication;

  cr.entityIdVector.push_back(&entityId);
  cr.entityIdVectorPresent = true;

  cr.contextRegistrationAttributeVector.push_back(&attribute);
  rcrP->contextRegistrationVector.push_back(&cr);

  // Now call postRegisterContext (postRegisterContext doesn't use the parameters 'components' and 'compV')
  answer = postRegisterContext(ciP, components, compV, parseDataP);
  cr.registrationMetadataVector.release();

  return answer;
}
