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
std::string postUpdateContext(ConnectionInfo* ciP, int components, std::vector<std::string>& compV, ParseData* parseDataP)
{
  UpdateContextResponse  upcr;
  std::string            answer;

  ciP->httpStatusCode = mongoUpdateContext(&parseDataP->upcr.res, &upcr, ciP->tenant, ciP->servicePathV);

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
    answer = upcr.render(UpdateContext, ciP->outFormat, "");
    return answer;
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

    //
    // FIXME P9: Instead of taking the contextElement from 'upcr', which is the output from mongoUpdateContext,
    //           I take it from 'parseDataP->upcr.res', which is the input to mongoUpdateContext, and the
    //           input from the original sender.
    //           I tried with 'upcr', but there is no contextAttributeVector there, so I can't use it.
    //           The problem with using 'parseDataP->upcr.res' is that I use only parseDataP->upcr.res.contextElementVector[0],
    //           while there may be more than one contextElement in the request and I don't know to which the response of 
    //           mongoUpdateContext corresponds.
    //           Assuming there is only ONE contextElement all should be OK.
    //
    //           HOWEVER, we should take a close look at this during the PR review.
    //
    //           What I would like to do here:
    //           ceP->fill(upcr.contextElementResponseVector[ix]->contextElement);
    //
    ceP->fill(parseDataP->upcr.res.contextElementVector[0]);
    ucrP->contextElementVector.push_back(ceP);


    //
    // 3. Render an XML-string of the request we want to forward
    //
    std::string payloadIn = ucrP->render(UpdateContext, XML, "");


    //
    // 4. Forward the query to the providing application
    // FIXME P7: Should Rush be used?
    //
    std::string     out;
    std::string     verb         = "POST";
    std::string     resource     = prefix + "/updateContext";
    std::string     tenant       = ciP->tenant;
    
    out = sendHttpSocket(ip, port, protocol, verb, tenant, resource, "application/xml", payloadIn, false, true);

    // Should be safe to free up ucrP now ...
    ucrP->release();
    delete ucrP;

    if ((out == "error") || (out == ""))
    {
      std::string details = "error forwardingupdateContext to " + ip + ":" + portV + resource + ": " + out;
      cerP->statusCode.fill(SccReceiverInternalError, details);
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
    
    s = xmlTreat(cleanPayload, ciP, &parseData, RtUpdateContextResponse, "updateContextResponse", NULL, &errorMsg);
    provUpcrsP = &parseData.upcrs.res;
    if (s != "OK")
    {
      std::string details = "error forwarding 'Update' to providing application " + ip + portV + resource + ": " + "error parsing XML";
      
      cerP->statusCode.fill(SccReceiverInternalError, details);
      LM_E(("Runtime Error (%s)", details.c_str()));
      continue;
    }


    //
    // 6. Replace the ContextElementResponse that was "Found" with the info in the UpdateContextResponse
    //
    cerP->contextElement.fill(provUpcrsP->contextElementResponseVector[0]->contextElement);
    cerP->statusCode.fill(&provUpcrsP->contextElementResponseVector[0]->statusCode);

    if (cerP->statusCode.details == "")
    {
      cerP->statusCode.details = "Redirected to context provider " + ip + ":" + portV + prefix;
    }
  }

  answer = upcr.render(UpdateContext, ciP->outFormat, "");
  return answer;
}
