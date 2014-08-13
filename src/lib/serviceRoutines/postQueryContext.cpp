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
std::string postQueryContext(ConnectionInfo* ciP, int components, std::vector<std::string>& compV, ParseData* parseDataP)
{
  QueryContextResponse  qcr;
  std::string           answer;
   
  ciP->httpStatusCode = mongoQueryContext(&parseDataP->qcr.res, &qcr, ciP->tenant, ciP->servicePathV, ciP->uriParam);

  LM_M(("kz. IN postQueryContext. mongoQueryContext response: %d", ciP->httpStatusCode));
  LM_M(("kz. IN postQueryContext. errorCode.code:             %d", qcr.errorCode.code));

  // If no redirectioning is necessary, just return the result
  if (qcr.errorCode.code != SccFound)
  {
    answer = qcr.render(QueryContext, ciP->outFormat, "");
    return answer;
  }



  std::string     ip;
  std::string     protocol;
  int             port;
  std::string     prefix;

  LM_M(("kz. Got a FOUND"));

  //
  // A has been found on some context provider.
  // We need to forward the query request to the context provider, indicated in qcr.errorCode.details.
  //
  // 1. Parse the providing application to extract IP, port and URI-path
  // 2. The exact same QueryContextRequest will be used for the query-forwarding
  // 3. Render an XML-string of the request we want to forward
  // 4. Send the request to the providing application (and await the response)
  // 5. Parse the XML response and fill in a binary QueryContextResponse
  // 6. Replace the ContextElementResponse that was "Found" with the info in the QueryContextResponse
  //


  //
  // 1. Parse the providing application to extract IP, port and URI-path
  //
  if (parseUrl(qcr.errorCode.details, ip, port, prefix, protocol) == false)
  {
    QueryContextResponse qcrs;

    LM_W(("Bad Input (invalid proving application '%s')", qcr.errorCode.details.c_str()));
    // FIXME P9: 500 SccReceiverInternalError or 400 SccBadRequest ?
    //           Somehow, if the accepted this providing application, it is the brokers fault ...
    //           SccBadRequest should have been returned before, when it was registered!

    qcrs.errorCode.fill(SccReceiverInternalError, "error parsing providing application");
    answer = qcrs.render(QueryContext, ciP->outFormat, "");
    return answer;
  }
  LM_M(("kz. Providing App: %s POST %s:%d%s/ngsi/queryContext", protocol.c_str(), ip.c_str(), port, prefix.c_str()));


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

    qcrs.errorCode.fill(SccReceiverInternalError, "error rendering forward-request");
    answer = qcrs.render(QueryContext, ciP->outFormat, "");
    return answer;
  }
  LM_M(("kz. payload to send to providing application:\n%s", cleanPayload));


  //
  // 4. Send the request to the providing application (and await the reply)
  // FIXME P7: Should Rush be used?
  //
  std::string     out;
  std::string     verb         = "POST";
  std::string     resource     = prefix + "/ngsi10/queryContext";
  std::string     tenant       = ciP->tenant;

  out = sendHttpSocket(ip, port, protocol, verb, tenant, resource, "application/xml", payload, false, true);
  LM_M(("kz. sendHttpSocket returned:\n%s", out.c_str()));

  if ((out == "error") || (out == ""))
  {
    QueryContextResponse qcrs;

    qcrs.errorCode.fill(SccReceiverInternalError, "error forwarding query to providing application,");
    answer = qcrs.render(QueryContext, ciP->outFormat, "");
    return answer;
  }


  ParseData    parseData;
  std::string  s;
  std::string  errorMsg;

  cleanPayload = xmlPayloadClean(out.c_str(), "<queryContextResponse>");
  LM_M(("kz. payload received from providing application:\n%s", cleanPayload));

  s = xmlTreat(cleanPayload, ciP, &parseData, RtQueryContextResponse, "queryContextResponse", NULL, &errorMsg);
  LM_M(("kz. xmlTreat returned:\n%s", s.c_str()));
  if (s != "OK")
  {
    QueryContextResponse qcrs;

    qcrs.errorCode.fill(SccReceiverInternalError, std::string("treating reply from prov app: ") + errorMsg);
    LM_W(("Internal Error (error parsing reply from prov app: %s)", errorMsg.c_str()));
    answer = qcrs.render(QueryContext, ciP->outFormat, "");
    return answer;
  }
  LM_M(("------------------------ XML Treat OK (kz) -----------------------------"));

  char portV[16];
  snprintf(portV, sizeof(portV), "%d", port);

  // Fill in the response from the redirection into the response to the originator of this request
  QueryContextResponse* qcrsP = &parseData.qcrs.res;

  LM_M(("kz. Got %d Context Element Responses", qcrsP->contextElementResponseVector.size()));
  LM_M(("kz. Got errorCode %d (%s), details '%s'", qcrsP->errorCode.code, qcrsP->errorCode.reasonPhrase.c_str(), qcrsP->errorCode.details.c_str()));

  answer = qcrsP->render(QueryContext, ciP->outFormat, "");
  return answer;
}

