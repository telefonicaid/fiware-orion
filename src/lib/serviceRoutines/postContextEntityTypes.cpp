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
#include "ngsi9/RegisterContextResponse.h"
#include "rest/ConnectionInfo.h"
#include "serviceRoutines/postRegisterContext.h"  // instead of convenienceMap function, postRegisterContext is used
#include "serviceRoutines/postContextEntityTypes.h"



/* ****************************************************************************
*
* postContextEntityTypes - 
*
* POST /ngsi9/contextEntityTypes/{entityType}
*/
std::string postContextEntityTypes(ConnectionInfo* ciP, int components, std::vector<std::string> compV, ParseData* parseDataP)
{
  std::string  entityType    = compV[2];
  
  // Transform RegisterProviderRequest into RegisterContextRequest
  parseDataP->rcr.res.fill(parseDataP->rpr.res, "", entityType, "");

  // Now call postRegisterContext (postRegisterContext doesn't use the parameters 'components' and 'compV')
  std::string answer = postRegisterContext(ciP, components, compV, parseDataP);
  parseDataP->rpr.res.release();

  return answer;
}
