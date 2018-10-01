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
#include <vector>

#include "logMsg/logMsg.h"

#include "common/tag.h"
#include "alarmMgr/alarmMgr.h"
#include "ngsi/ContextAttributeVector.h"
#include "ngsi/StatusCode.h"
#include "ngsi10/QueryContextResponse.h"
#include "ngsi/Request.h"
#include "convenience/ContextAttributeResponse.h"
#include "rest/ConnectionInfo.h"



/* ****************************************************************************
*
* toJsonV1 -
*/
std::string ContextAttributeResponse::toJsonV1
(
  bool         asJsonObject,
  RequestType  request
)
{
  std::string out = "";

  // No metadata filter in this case, an empty vector is used to fulfil method signature.
  // For attribute filter, we use the ContextAttributeVector itself
  std::vector<std::string> emptyMdV;

  out += startTag();
  out += contextAttributeVector.toJsonV1(asJsonObject, request, contextAttributeVector.vec, emptyMdV, true);
  out += statusCode.toJsonV1(false);
  out += endTag();

  return out;
}



/* ****************************************************************************
*
* check - 
*/
std::string ContextAttributeResponse::check
(
  ApiVersion          apiVersion,
  bool                asJsonObject,
  RequestType         requestType,
  const std::string&  predetectedError
)
{
  std::string  res;

  if (predetectedError != "")
  {
    statusCode.fill(SccBadRequest, predetectedError);
  }
  else if ((res = contextAttributeVector.check(apiVersion, requestType)) != "OK")
  {
    std::string details = std::string("contextAttributeVector: '") + res + "'";
    alarmMgr.badInput(clientIp, details);
    statusCode.fill(SccBadRequest, res);

    //
    // If this ContextAttributeResponse is part of an IndividualContextEntity, the complete rendered
    // response is not desired, just the string returned from the check method
    //
    if (requestType == IndividualContextEntity)
    {
      return res;
    }
  }
  else
  {
    return "OK";
  }

  return toJsonV1(asJsonObject, requestType);
}



/* ****************************************************************************
*
* release - 
*/
void ContextAttributeResponse::release(void)
{
  contextAttributeVector.release();
}



/* ****************************************************************************
*
* fill - 
*/
void ContextAttributeResponse::fill(const ContextAttributeVector& caV, const StatusCode& _statusCode)
{
  contextAttributeVector.fill(caV);
  statusCode.fill(_statusCode);
}


/* ****************************************************************************
*
* ContextAttributeResponse::fill - 
*/
void ContextAttributeResponse::fill
(
  QueryContextResponse*  qcrP,
  const std::string&     entityId,
  const std::string&     entityType,
  const std::string&     attributeName
)
{
  if (qcrP == NULL)
  {
    statusCode.fill(SccContextElementNotFound);
    return;
  }

  if (qcrP->contextElementResponseVector.size() == 0)
  {
    statusCode.fill(&qcrP->errorCode);

    if ((statusCode.code == SccOk) || (statusCode.code == SccNone))
    {
      statusCode.fill(SccContextElementNotFound, "");
    }

    if ((statusCode.code != SccOk) && (statusCode.details == ""))
    {
      statusCode.details = "Entity-Attribute pair: /" + entityId + "-" + attributeName + "/";
    }

    return;
  }


  //
  // FIXME P7: If more than one context element is found, we simply select the first one.
  //           A better approach would be to change this convop to return a vector of responses.
  //           Adding a call to alarmMgr::badInput - with this I mean that the user that sends the 
  //           query needs to avoid using this conv op to make any queries that can give more than
  //           one unique context element :-).
  //           This FIXME is related to github issue #588 and (probably) #650.
  //           Also, optimizing this would be part of issue #768
  //
  if (qcrP->contextElementResponseVector.size() > 1)
  {
    alarmMgr.badInput(clientIp, "more than one context element found in this query - selecting the first one");
  }

  contextAttributeVector.fill(qcrP->contextElementResponseVector[0]->entity.attributeVector);

  if ((statusCode.code == SccNone) || (statusCode.code == SccOk))
  {
    if (qcrP->errorCode.code == SccNone)
    {
      // Fix code, preserve details
      qcrP->errorCode.fill(SccOk, qcrP->errorCode.details);
    }

    statusCode.fill(&qcrP->errorCode);
  }
}
