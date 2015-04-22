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

#include "common/Format.h"
#include "common/tag.h"
#include "ngsi/ContextElementResponse.h"
#include "ngsi10/QueryContextResponse.h"
#include "rest/ConnectionInfo.h"



/* ****************************************************************************
*
* ContextElementResponse::ContextElementResponse -
*/
ContextElementResponse::ContextElementResponse()
{
  prune = false;
}



/* ****************************************************************************
*
* ContextElementResponse::ContextElementResponse - 
*/
ContextElementResponse::ContextElementResponse(EntityId* eP, ContextAttribute* aP)
{
  prune = false;

  contextElement.entityId.fill(eP);

  if (aP != NULL)
  {
    contextElement.contextAttributeVector.push_back(new ContextAttribute(aP));
  }
}



/* ****************************************************************************
*
* ContextElementResponse::ContextElementResponse - 
*/
ContextElementResponse::ContextElementResponse(ContextElementResponse* cerP)
{
  prune = false;

  contextElement.fill(cerP->contextElement);
  statusCode.fill(cerP->statusCode);
}



/* ****************************************************************************
*
* ContextElementResponse::render - 
*/
std::string ContextElementResponse::render
(
  ConnectionInfo*     ciP,
  RequestType         requestType,
  const std::string&  indent,
  bool                comma,
  bool                omitAttributeValues
)
{
  std::string xmlTag   = "contextElementResponse";
  std::string jsonTag  = "contextElement";
  std::string out      = "";

  out += startTag(indent, xmlTag, jsonTag, ciP->outFormat, false, false);
  out += contextElement.render(ciP, requestType, indent + "  ", true, omitAttributeValues);
  out += statusCode.render(ciP->outFormat, indent + "  ", false);
  out += endTag(indent, xmlTag, ciP->outFormat, comma, false);

  return out;
}



/* ****************************************************************************
*
* ContextElementResponse::release - 
*/
void ContextElementResponse::release(void)
{
  contextElement.release();
  statusCode.release();
}



/* ****************************************************************************
*
* ContextElementResponse::check - 
*/
std::string ContextElementResponse::check
(
  RequestType         requestType,
  Format              format,
  const std::string&  indent,
  const std::string&  predetectedError,
  int                 counter
)
{
  std::string res;

  if ((res = contextElement.check(requestType, format, indent, predetectedError, counter)) != "OK")
  {
    return res;
  }

  if ((res = statusCode.check(requestType, format, indent, predetectedError, counter)) != "OK")
  {
    return res;
  }

  return "OK";
}



/* ****************************************************************************
*
* ContextElementResponse::present - 
*/
void ContextElementResponse::present(const std::string& indent, int ix)
{
  LM_F(("%sContextElementResponse %d:", indent.c_str(), ix));
  contextElement.present(indent + "  ", ix);
  statusCode.present(indent + "  ");
}



/* ****************************************************************************
*
* ContextElementResponse::fill - 
*/
void ContextElementResponse::fill(QueryContextResponse* qcrP, const std::string& entityId, const std::string& entityType)
{
  if (qcrP == NULL)
  {
    statusCode.fill(SccContextElementNotFound);
    return;
  }

  if (qcrP->contextElementResponseVector.size() == 0)
  {
    statusCode.fill(&qcrP->errorCode);
    contextElement.entityId.fill(entityId, entityType, "false");

    if ((statusCode.code != SccOk) && (statusCode.details == ""))
    {
      statusCode.details = "Entity id: /" + entityId + "/";
    }

    return;
  }

  //
  // FIXME P7: If more than one context element is found, we simply select the first one.
  //           A better approach would be to change this convop to return a vector of responses.
  //           Adding a warning with 'Bad Input' - with this I mean that the user that sends the 
  //           query needs to avoid using this conv op to make any queries that can give more than
  //           one unique context element :-).
  //           This FIXME is related to github issue #588 and (probably) #650.
  //           Also, optimizing this would be part of issue #768
  //
  if (qcrP->contextElementResponseVector.size() > 1)
  {
    LM_W(("Bad Input (more than one context element found the this query - selecting the first one"));
  }

  contextElement.fill(&qcrP->contextElementResponseVector[0]->contextElement);

  if (qcrP->errorCode.code != SccNone)
  {
    statusCode.fill(&qcrP->errorCode);
  }
}



/* ****************************************************************************
*
* ContextElementResponse::fill - 
*/
void ContextElementResponse::fill(ContextElementResponse* cerP)
{
  contextElement.fill(cerP->contextElement);
  statusCode.fill(cerP->statusCode);
}



/* ****************************************************************************
*
* ContextElementResponse::clone - 
*/
ContextElementResponse* ContextElementResponse::clone(void)
{
  ContextElementResponse* cerP = new ContextElementResponse();

  cerP->fill(this);

  return cerP;
}
