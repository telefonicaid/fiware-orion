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
* render - 
*/
std::string ContextAttributeResponse::render
(
  const std::string&  apiVersion,
  bool                asJsonObject,
  RequestType         request,
  const std::string&  indent
)
{
  std::string out = "";

  out += startTag(indent);
  out += contextAttributeVector.render(apiVersion, asJsonObject, request, indent + "  ", true);
  out += statusCode.render(indent + "  ");
  out += endTag(indent);

  return out;
}



/* ****************************************************************************
*
* check - 
*/
std::string ContextAttributeResponse::check
(
  const std::string&  apiVersion,
  bool                asJsonObject,
  RequestType         requestType,
  std::string         indent,
  std::string         predetectedError,
  int                 counter
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

  return render(apiVersion, asJsonObject, requestType, indent);
}



/* ****************************************************************************
*
* present - 
*/
void ContextAttributeResponse::present(std::string indent)
{
  contextAttributeVector.present(indent);
  statusCode.present(indent);
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
void ContextAttributeResponse::fill(ContextAttributeVector* _cavP, const StatusCode& _statusCode)
{
  contextAttributeVector.fill(_cavP);
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
  const std::string&     attributeName,
  const std::string&     metaID
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
      if (metaID == "")
        statusCode.details = "Entity-Attribute pair: /" + entityId + "-" + attributeName + "/";
      else
        statusCode.details = "Entity-Attribute-MetaID triplet: /" + entityId + "-" + attributeName + "-" + metaID + "/";
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

  //
  // If we have to match against Metadata::ID, then we have to through the entire ContextAttribute vector
  // of the Context Element to find matches.
  //
  // If there is no metaID (metaID == ""), then we simply copy the vector
  //
  if (metaID != "")
  {
    for (unsigned int aIx = 0; aIx < qcrP->contextElementResponseVector[0]->contextElement.contextAttributeVector.size(); ++aIx)
    {
      ContextAttribute* caP  = qcrP->contextElementResponseVector[0]->contextElement.contextAttributeVector[aIx];
      Metadata*         mP   = caP->metadataVector.lookupByName("ID");

      if ((mP == NULL) || (mP->stringValue != metaID))
      {
        continue;
      }
      contextAttributeVector.push_back(caP->clone());
    }

    if (contextAttributeVector.size() == 0)
    {
      std::string details = "Entity-Attribute-MetaID triplet: /" + entityId + "-" + attributeName + "-" + metaID + "/";
      statusCode.fill(SccContextElementNotFound, details);
    }
  }
  else
  {
    contextAttributeVector.fill(&qcrP->contextElementResponseVector[0]->contextElement.contextAttributeVector);
  }

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
