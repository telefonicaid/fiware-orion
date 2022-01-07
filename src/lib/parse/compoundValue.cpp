/*
*
* Copyright 2014 Telefonica Investigacion y Desarrollo, S.A.U
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
#include "alarmMgr/alarmMgr.h"

#include "orionTypes/OrionValueType.h"
#include "ngsi/ParseData.h"
#include "rest/RestService.h"
#include "parse/CompoundValueNode.h"
#include "parse/compoundValue.h"



namespace orion
{
/* ****************************************************************************
*
* compoundValueEnd - 
*/
void compoundValueEnd(ConnectionInfo* ciP, ParseData* parseDataP)
{
  LM_T(LmtCompoundValue, ("Compound END"));

  // Finish the compound value - error check included
  std::string status = ciP->compoundValueRoot->finish();

  // Any problems in 'finish'?
  // If so, mark as erroneous
  if (status != "OK")
  {
    ciP->httpStatusCode = SccBadRequest;
    ciP->answer = std::string("compound value error: ") + status;
    alarmMgr.badInput(clientIp, ciP->answer);
  }

  //
  // Give the root pointer of this Compound to the active ContextAttribute
  // lastContextAttribute is set in the JSON v1 parsing routines, to point at the
  // latest contextAttribute, i.e. the attribute whose 'contextValue' is the
  // owner of this compound value tree.
  //

  LM_T(LmtCompoundValue, ("Set compoundValueP (%p) for attribute at %p",
                          ciP->compoundValueRoot,
                          parseDataP->lastContextAttribute));

  //
  // Special case for updateContextAttributeRequest. This payload has no
  // ContextAttribute to point to by lastContextAttribute, as the whole payload
  // is a part of a ContextAttribute.
  //
  RequestType requestType = ciP->restServiceP->request;
  
  if ((requestType == AttributeValueInstance)                           ||
      (requestType == AttributeValueInstanceWithTypeAndId)              ||
      (requestType == IndividualContextEntityAttribute)                 ||
      (requestType == IndividualContextEntityAttributeWithTypeAndId))
  {
    parseDataP->upcar.res.compoundValueP = ciP->compoundValueRoot;
  }
  else
  {
    parseDataP->lastContextAttribute->compoundValueP = ciP->compoundValueRoot;
  }

  // Reset the Compound stuff in ConnectionInfo
  ciP->compoundValueRoot = NULL;
  ciP->compoundValueP    = NULL;
  LM_T(LmtCompoundValueContainer, ("Set current container to NULL"));
  ciP->inCompoundValue   = false;
}
}
