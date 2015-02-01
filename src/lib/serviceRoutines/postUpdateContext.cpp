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
#include "logMsg/traceLevels.h"
#include "common/string.h"
#include "mongoBackend/mongoUpdateContext.h"
#include "ngsi/ParseData.h"
#include "ngsi10/UpdateContextResponse.h"
#include "rest/ConnectionInfo.h"
#include "rest/clientSocketHttp.h"
#include "serviceRoutines/postUpdateContext.h"
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
* postUpdateContext -
*/
std::string postUpdateContext
(
  ConnectionInfo*            ciP,
  int                        components,
  std::vector<std::string>&  compV,
  ParseData*                 parseDataP
)
{
  UpdateContextResponse  upcr;
  std::string            answer;

  ciP->httpStatusCode = mongoUpdateContext(&parseDataP->upcr.res, &upcr, ciP->tenant, ciP->servicePathV, ciP->uriParam, ciP->httpHeaders.xauthToken, "postUpdateContext");

  //
  // Checking for SccFound in the ContextElementResponseVector
  // Any 'Founds' must be forwarded to their respective providing applications
  //

  //
  // If upcr.errorCode contains an error after mongoUpdateContext, we return a 404 Not Found.
  // Right now, this only happens if there is more than one servicePath, but in the future this may grow
  //
  if ((upcr.errorCode.code != SccOk) && (upcr.errorCode.code != SccNone))
  {
    upcr.errorCode.fill(SccContextElementNotFound, "");
    answer = upcr.render(ciP, UpdateContext, "");
    return answer;
  }

  //
  // If upcr.contextElementResponseVector contains >= TWO responses and the second of them is a 'SccFound',
  // then skip the first, the one that isn't 'SccFound'.
  // Also, if the first of them is 472, then skip the second
  //
  // FIXME P6: Temporary hack for 'Entity found but Attribute not found':
  //           See FIXME P6 'Temporary hack ...' 
  //           in MongoCommonUpdate.cpp, function processContextElement
  //


  //
  // Removing 'garbage' from the response vector (when just *one* incoming contextElement
  //
  
  if (upcr.contextElementResponseVector.size() != parseDataP->upcr.res.contextElementVector.size())
  {
    for (unsigned int ix = 0; ix < upcr.contextElementResponseVector.size() - 1; ++ix)
    {
      //
      // 2 cases:
      //
      // o 1. 472+302:  keep 302
      //   Out[0]: (472) request parameter is invalid/not allowed
      //   Out[1]: (302) Found
      //
      // o 2. 472+404: keep 472
      //   Out[0]: (472) request parameter is invalid/not allowed
      //   Out[1]: (404) No context element found
      //
      if (upcr.contextElementResponseVector[ix]->statusCode.code == 472)
      {
        if (ix + 1 < upcr.contextElementResponseVector.size())
        {
          // 472+302
          if (upcr.contextElementResponseVector[ix + 1]->statusCode.code == 302)
          {
            // Remove the 472
            upcr.contextElementResponseVector.vec.erase(upcr.contextElementResponseVector.vec.begin() + ix);
          }

          // 472+404
          else if (upcr.contextElementResponseVector[ix + 1]->statusCode.code == 404)
          {
            // Remove the 404
            upcr.contextElementResponseVector.vec.erase(upcr.contextElementResponseVector.vec.begin() + ix + 1);
          }
        }
      }
    }
  }


  for (unsigned int ix = 0; ix < upcr.contextElementResponseVector.size(); ++ix)
  {
    ContextElementResponse* cerP = upcr.contextElementResponseVector[ix];

    // If the statusCode.code is not SccFound, we leave the ContextElementResponse exactly the way it is
    if (cerP->statusCode.code != SccFound)
    {
      continue;
    }

    //
    // If the statusCode.code IS SccFound, it means we can find the contextElement elsewhere.
    //
    // 1. Parse the URL of the providing application in statusCode.details
    // 2. Fill in a 'new' ContextElement for an UpdateContextRequest (action from parseDataP->upcr.res.updateActionType)
    // 3. Render an XML-string of the request we want to forward
    // 4. Forward the query to the providing application
    // 5. Parse the XML response and fill in a binary ContextElementResponse
    // 6. Replace the ContextElementResponse that was "Found" with the info in the UpdateContextResponse
    //


    //
    // 1. Parse the URL of the providing application in statusCode.details
    //
    std::string     ip;
    std::string     protocol;
    int             port;
    std::string     prefix;

    if (parseUrl(cerP->statusCode.details, ip, port, prefix, protocol) == false)
    {
      cerP->statusCode.fill(SccReceiverInternalError, "error parsing providing application");
      continue;
    }

    // Saving port as a string - portV will be used composing detailed error messages
    char portV[16];
    snprintf(portV, sizeof(portV), "%d", port);


    //
    // 2. Fill in a 'new' ContextElement for an UpdateContextRequest
    //
    UpdateContextRequest*  ucrP = new UpdateContextRequest();
    ContextElement*        ceP  = new ContextElement();

    ucrP->updateActionType      = parseDataP->upcr.res.updateActionType;
    ucrP->contextElementVector.push_back(ceP);
    ceP->fill(upcr.contextElementResponseVector[ix]->contextElement);



    //
    // 3. Render a string of the request we want to forward, forced to XML
    //    FIXME P8: The format of this string (XML or JSON) should depend on the
    //              CB-CPr content type negotiation, but right now we force it to XML.
    //
    ConnectionInfo ci;

    ci.outFormat = XML;

    std::string payloadIn = ucrP->render(&ci, UpdateContext, "");

    LM_T(LmtCtxProviders, ("payloadIn:\n%s", payloadIn.c_str()));

    //
    // 4. Forward the query to the providing application
    // FIXME P7: Should Rush be used?
    //
    std::string     out;
    std::string     verb         = "POST";
    std::string     resource     = prefix + "/updateContext";
    std::string     tenant       = ciP->tenant;

    out = sendHttpSocket(ip, 
                         port,
                         protocol,
                         verb,
                         tenant,
                         ciP->httpHeaders.servicePath,
                         ciP->httpHeaders.xauthToken,
                         resource,
                         "application/xml",
                         payloadIn,
                         false,
                         true);

    // Should be safe to free up ucrP now ...
    ucrP->release();
    delete ucrP;

    if ((out == "error") || (out == ""))
    {
      std::string details = "error forwardingupdateContext to " + ip + ":" + portV + resource + ": " + out;
      cerP->statusCode.fill(SccContextElementNotFound, "");
      LM_E(("Runtime Error (error forwarding 'Update' to providing application)"));
      continue;
    }


    //
    // 5. Parse the XML response and fill in a binary ContextElementResponse
    //
    ParseData               parseData;
    std::string             s;
    std::string             errorMsg;
    char*                   cleanPayload;
    UpdateContextResponse*  provUpcrsP;

    cleanPayload = xmlPayloadClean(out.c_str(), "<updateContextResponse>");

    if ((cleanPayload == NULL) || (cleanPayload[0] == 0))
    {
      UpdateContextResponse ucrs;

      //
      // This is really an internal error in the Context Provider
      // It is not in the orion broker though, so 404 is returned
      //
      ucrs.errorCode.fill(SccContextElementNotFound, "invalid context provider response");

      LM_W(("Other Error (context provider response to UpdateContext is empty)"));
      answer = ucrs.render(ciP, UpdateContext, "");
      return answer;
    }

    s = xmlTreat(cleanPayload, ciP, &parseData, RtUpdateContextResponse, "updateContextResponse", NULL, &errorMsg);
    provUpcrsP = &parseData.upcrs.res;
    if (s != "OK")
    {
      std::string details = "error forwarding 'Update' to providing application " +
        ip + portV + resource + ": " + "error parsing XML";

      cerP->statusCode.fill(SccContextElementNotFound, "");
      LM_E(("Runtime Error (%s)", details.c_str()));
      continue;
    }


    //
    // 6. Replace the ContextElementResponse that was "Found" with the info in the UpdateContextResponse
    //
    cerP->contextElement.release();
    cerP->contextElement.fill(provUpcrsP->contextElementResponseVector[0]->contextElement);
    cerP->statusCode.fill(&provUpcrsP->contextElementResponseVector[0]->statusCode);

    if (cerP->statusCode.details == "")
    {
      cerP->statusCode.details = "Redirected to context provider " + ip + ":" + portV + prefix;
    }
  }

  answer = upcr.render(ciP, UpdateContext, "");
  return answer;
}
