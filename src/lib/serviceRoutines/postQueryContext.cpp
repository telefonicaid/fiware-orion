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

#include "common/string.h"
#include "mongoBackend/mongoQueryContext.h"
#include "ngsi/ParseData.h"
#include "ngsi10/QueryContextResponse.h"
#include "rest/ConnectionInfo.h"
#include "rest/clientSocketHttp.h"
#include "serviceRoutines/postQueryContext.h"
#include "xmlParse/xmlRequest.h"



/* ****************************************************************************
*
* xmlPayloadClean -
*/
static char* xmlPayloadClean(const char*  payload, const char* payloadWord)
{
  return (char*) strstr(payload, payloadWord);
}



/* ****************************************************************************
*
* postQueryContext -
*/
std::string postQueryContext
(
  ConnectionInfo*            ciP,
  int                        components,
  std::vector<std::string>&  compV,
  ParseData*                 parseDataP
)
{
  QueryContextResponse  qcr;
  std::string           answer;

  ciP->httpStatusCode = mongoQueryContext(&parseDataP->qcr.res, &qcr, ciP->tenant, ciP->servicePathV, ciP->uriParam);

  // If no redirectioning is necessary, just return the result
  if (qcr.errorCode.code != SccFound)
  {
    answer = qcr.render(ciP, QueryContext, "");
    return answer;
  }



  std::string     ip;
  std::string     protocol;
  int             port;
  std::string     prefix;

  //
  // An entity/attribute has been found on some context provider.
  // We need to forward the query request to the context provider, indicated in qcr.errorCode.details.
  //
  // 1. Parse the providing application to extract IP, port and URI-path
  // 2. The exact same QueryContextRequest will be used for the query-forwarding
  // 3. Render an XML-string of the request we want to forward
  // 4. Send the request to the providing application (and await the response)
  // 5. Parse the XML response and fill in a binary QueryContextResponse
  // 6. Render QueryContextResponse from the providing application
  //


  //
  // 1. Parse the providing application to extract IP, port and URI-path
  //
  if (parseUrl(qcr.errorCode.details, ip, port, prefix, protocol) == false)
  {
    QueryContextResponse qcrs;

    LM_W(("Bad Input (invalid proving application '%s')", qcr.errorCode.details.c_str()));
    //  Somehow, if we accepted this providing application, it is the brokers fault ...
    //  SccBadRequest should have been returned before, when it was registered!

    qcrs.errorCode.fill(SccContextElementNotFound, "");
    answer = qcrs.render(ciP, QueryContext, "");
    return answer;
  }


  //
  // 2. The exact same QueryContextRequest will be used for the query-forwarding
  //
  QueryContextRequest* qcReqP = &parseDataP->qcr.res;


  //
  // 3. Render an XML-string of the request we want to forward
  //
  std::string payload = qcReqP->render(QueryContext, XML, "");
  char*       cleanPayload;

  if ((cleanPayload = xmlPayloadClean(payload.c_str(), "<queryContextRequest>")) == NULL)
  {
    QueryContextResponse qcrs;

    LM_E(("Runtime Error (error rendering forward-request)"));
    qcrs.errorCode.fill(SccContextElementNotFound, "");
    answer = qcrs.render(ciP, QueryContext, "");
    return answer;
  }


  //
  // 4. Send the request to the providing application (and await the reply)
  // FIXME P7: Should Rush be used?
  //
  std::string     out;
  std::string     verb         = "POST";
  std::string     resource     = prefix + "/queryContext";
  std::string     tenant       = ciP->tenant;

  // FIXME P5: Service-Path ("" after 'tenant' in call to sendHttpSocket)
  out = sendHttpSocket(ip, port, protocol, verb, tenant, "", resource, "application/xml", payload, false, true);

  if ((out == "error") || (out == ""))
  {
    QueryContextResponse qcrs;

    qcrs.errorCode.fill(SccContextElementNotFound, "");
    answer = qcrs.render(ciP, QueryContext, "");
    LM_E(("Runtime Error (error forwarding 'Query' to providing application)"));
    return answer;
  }


  //
  // 5. Parse the XML response and fill in a binary QueryContextResponse
  //
  ParseData    parseData;
  std::string  s;
  std::string  errorMsg;

  cleanPayload = xmlPayloadClean(out.c_str(), "<queryContextResponse>");

  s = xmlTreat(cleanPayload, ciP, &parseData, RtQueryContextResponse, "queryContextResponse", NULL, &errorMsg);
  if (s != "OK")
  {
    QueryContextResponse qcrs;

    qcrs.errorCode.fill(SccContextElementNotFound, "");
    LM_W(("Internal Error (error parsing reply from prov app: %s)", errorMsg.c_str()));
    answer = qcrs.render(ciP, QueryContext, "");
    return answer;
  }


  //
  // 6. Render QueryContextResponse from the providing application
  //
  char portV[16];
  snprintf(portV, sizeof(portV), "%d", port);

  // Fill in the response from the redirection into the response to the originator of this request
  QueryContextResponse* qcrsP = &parseData.qcrs.res;

  //
  // Returning 'redirected to' in StatusCode::details
  //
  if (((qcrsP->errorCode.code != SccOk) && (qcrsP->errorCode.code != SccNone)) && (qcrsP->errorCode.details == ""))
  {
    qcrsP->errorCode.details = "Redirected to context provider " + ip + ":" + portV + prefix;
  }
  else if ((qcrsP->contextElementResponseVector.size() > 0) &&
           (qcrsP->contextElementResponseVector[0]->statusCode.details == ""))
  {
    qcrsP->contextElementResponseVector[0]->statusCode.details =
      "Redirected to context provider " + ip + ":" + portV + prefix;
  }

  answer = qcrsP->render(ciP, QueryContext, "");
  return answer;
}
