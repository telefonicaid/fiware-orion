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
#include "common/defaultValues.h"
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
*
* POST /v1/updateContext
* POST /ngsi10/updateContext
*
* Payload In:  UpdateContextRequest
* Payload Out: UpdateContextResponse
*/
std::string postUpdateContext
(
  ConnectionInfo*            ciP,
  int                        components,
  std::vector<std::string>&  compV,
  ParseData*                 parseDataP
)
{
  UpdateContextResponse*  upcrsP = &parseDataP->upcrs.res;
  std::string             answer;

  //
  // If more than ONE service-path is input, an error is returned as response.
  // If NO service-path is issued, then the default service-path "/" is used.
  // After these checks, the service-path is checked to be 'correct'.
  //
  if (ciP->servicePathV.size() > 1)
  {
    upcrsP->errorCode.fill(SccBadRequest, "more than one service path in context update request");
    LM_W(("Bad Input (more than one service path for an update request)"));
    answer = upcrsP->render(ciP, UpdateContext, "");
    return answer;
  }
  else if (ciP->servicePathV.size() == 0)
  {
    ciP->servicePathV.push_back(DEFAULT_SERVICE_PATH);
  }

  std::string res = servicePathCheck(ciP->servicePathV[0].c_str());
  if (res != "OK")
  {
    upcrsP->errorCode.fill(SccBadRequest, res);
    answer = upcrsP->render(ciP, UpdateContext, "");
    return answer;
  }


#if 1
  LM_M(("KZ '%s' %s:%s (%s:%s)", 
        parseDataP->upcr.res.updateActionType.c_str(),
        parseDataP->upcr.res.contextElementVector[0]->entityId.id.c_str(),
        parseDataP->upcr.res.contextElementVector[0]->entityId.type.c_str(),
        parseDataP->upcr.res.contextElementVector[0]->contextAttributeVector[0]->name.c_str(),
        parseDataP->upcr.res.contextElementVector[0]->contextAttributeVector[0]->type.c_str()));
#endif

  ciP->httpStatusCode = mongoUpdateContext(&parseDataP->upcr.res, upcrsP, ciP->tenant, ciP->servicePathV, ciP->uriParam, ciP->httpHeaders.xauthToken, "postUpdateContext");

  //
  // Checking for SccFound in the ContextElementResponseVector
  // Any 'Founds' must be forwarded to their respective providing applications
  //

  //
  // If upcrsP->errorCode contains an error after mongoUpdateContext, we return a 404 Not Found.
  // Right now, this only happens if there is more than one servicePath, but in the future this may grow
  //
  if ((upcrsP->errorCode.code != SccOk) && (upcrsP->errorCode.code != SccNone))
  {
    upcrsP->errorCode.fill(SccContextElementNotFound, "");
    answer = upcrsP->render(ciP, UpdateContext, "");
    return answer;
  }

  //
  // If upcrsP->contextElementResponseVector contains >= TWO responses and the second of them is a 'SccFound',
  // then skip the first, the one that isn't 'SccFound'.
  // Also, if the first of them is 472, then skip the second
  //
  // FIXME P6: Temporary hack for 'Entity found but Attribute not found':
  //           See FIXME P6 'Temporary hack ...' 
  //           in MongoCommonUpdate.cpp, function processContextElement
  //


  //
  // Removing 'garbage' from the response vector
  // [ When then number of incoming contextElement is different than the number of contextElement responses ]
  //
  // There are two different cases of 'garbage', as when a 472 is encountered, to allow for the search for a 302
  // (meaning that the entity/attribute is found is a ngsi9 registration), the search for the entity/attribute is 
  // not considered finalized, and later on the 472 is followed by either a 302 (Found) or a 404 (Not Found).
  //
  // The 302 gives more info than the 472 (especially as it makes us forward the request right here), while in the
  // other case, the 404 gives less info than the 472, so for the case of '472+302', the 302 must ge kept and for
  // '472+404', the 472 must be kept
  // 
  //
  
  if (upcrsP->contextElementResponseVector.size() != parseDataP->upcr.res.contextElementVector.size())
  {
    // Note that the loop ends at the contextElementResponse BEFORE the last contextElementResponse in the vector
    for (unsigned int ix = 0; ix < upcrsP->contextElementResponseVector.size() - 1; ++ix)
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
      if (upcrsP->contextElementResponseVector[ix]->statusCode.code == SccInvalidParameter)
      {
        if (ix + 1 < upcrsP->contextElementResponseVector.size())
        {
          // 472+302 (SccInvalidParameter+SccFound): Remove the 472
          if (upcrsP->contextElementResponseVector[ix + 1]->statusCode.code == SccFound)
          {
            upcrsP->contextElementResponseVector[ix]->release();
            delete(upcrsP->contextElementResponseVector[ix]);
            upcrsP->contextElementResponseVector.vec.erase(upcrsP->contextElementResponseVector.vec.begin() + ix);
          }

          // 472+404 (SccInvalidParameter+SccContextElementNotFound): Remove the 404
          else if (upcrsP->contextElementResponseVector[ix + 1]->statusCode.code == SccContextElementNotFound)
          {
            upcrsP->contextElementResponseVector[ix + 1]->release();
            delete(upcrsP->contextElementResponseVector[ix + 1]);
            upcrsP->contextElementResponseVector.vec.erase(upcrsP->contextElementResponseVector.vec.begin() + ix + 1);
            ix = ix + 1;
          }
        }
      }
    }
  }


  for (unsigned int ix = 0; ix < upcrsP->contextElementResponseVector.size(); ++ix)
  {
    ContextElementResponse* cerP = upcrsP->contextElementResponseVector[ix];

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
      LM_W(("Bad Input (providing application: '%s')", cerP->statusCode.details.c_str()));
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
    ceP->fill(upcrsP->contextElementResponseVector[ix]->contextElement);



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
    std::string     servicePath  = (ciP->httpHeaders.servicePathReceived == true)? ciP->httpHeaders.servicePath : "";

    out = sendHttpSocket(ip, 
                         port,
                         protocol,
                         verb,
                         tenant,
                         servicePath,
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

  answer = upcrsP->render(ciP, UpdateContext, "");
  return answer;
}
