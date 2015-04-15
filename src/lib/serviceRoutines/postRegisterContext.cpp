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

#include "common/globals.h"
#include "common/string.h"
#include "common/defaultValues.h"
#include "serviceRoutines/postRegisterContext.h"
#include "mongoBackend/mongoRegisterContext.h"
#include "mongoBackend/mongoConfManOperations.h"
#include "ngsi/ParseData.h"
#include "rest/ConnectionInfo.h"
#include "rest/clientSocketHttp.h"
#include "xmlParse/xmlRequest.h"



/* ****************************************************************************
*
* fordwardRegisterContext -
*
* NOTE
*   Used by registerContextForward
*/
static std::string fordwardRegisterContext
(
  char*               host,
  int                 port,
  const std::string&  tenant,
  const std::string&  xauthToken,
  const std::string&  payload,
  const std::string&  servicePath
)
{
  LM_T(LmtCm, ("forwarding registerContext to: host='%s', port=%d", fwdHost, fwdPort));
  LM_T(LmtCm, ("payload (content-type: application/xml): '%s'", payload.c_str()));
  std::string response = sendHttpSocket(fwdHost,
                                        fwdPort,
                                        "http:",
                                        "POST",
                                        tenant,
                                        servicePath,
                                        xauthToken,
                                        "ngsi9/registerContext",
                                        // FIXME P3: unhardwire content type
                                        std::string("application/xml"),
                                        payload,
                                        true,
                                        true);

  LM_T(LmtCm, ("response to forward registerContext: '%s'", response.c_str()));

  return response;
}



/* ****************************************************************************
*
* registerContextForward -
*
* NOTE
*   Used by postRegisterContext
*/
static void registerContextForward
(
  ConnectionInfo*           ciP,
  ParseData*                parseDataP,
  RegisterContextResponse*  rcrP
)
{
  /* Forward registerContext */
  if (parseDataP->rcr.res.registrationId.isEmpty())
  {
    /* New registration case */
    ciP->httpStatusCode  = mongoRegisterContext(&parseDataP->rcr.res, rcrP, ciP->uriParam, ciP->tenant, ciP->servicePathV[0]);

    std::string payload  = parseDataP->rcr.res.render(RegisterContext, ciP->inFormat, "");
    std::string response = fordwardRegisterContext(fwdHost, fwdPort, ciP->tenant, ciP->httpHeaders.xauthToken, payload, ciP->servicePathV[0]);

    if (response == "error")
    {
      LM_E(("Runtime Error (fordwarding of RegisterContext failed)"));
      return;
    }

    ParseData    responseData;
    XmlRequest*  reqP = NULL;
    const char*  payloadStart = strstr(response.c_str(), "<registerContextResponse>");

    if (payloadStart == NULL)
    {
      LM_E(("Runtime Error (<registerContextResponse> not found in fordwardRegisterContext response '%s')",
            response.c_str()));
      return;
    }

    std::string  s = xmlTreat(payloadStart, ciP, &responseData, RegisterResponse, "", &reqP);
    if (s != "OK")
    {
      LM_W(("Bad Input (error parsing registerContextResponse: %s)", s.c_str()));
    }
    else
    {
      std::string fwdRegId = responseData.rcrs.res.registrationId.get();
      LM_T(LmtCm, ("forward regId is: '%s'", fwdRegId.c_str()));
      mongoSetFwdRegId(rcrP->registrationId.get(), fwdRegId, ciP->tenant);
    }

    if (reqP != NULL)
    {
      reqP->release(&responseData);
    }
  }
  else
  {
    /* Update case */
    std::string fwdRegId = mongoGetFwdRegId(parseDataP->rcr.res.registrationId.get(), ciP->tenant);

    ciP->httpStatusCode  = mongoRegisterContext(&parseDataP->rcr.res, rcrP, ciP->uriParam, ciP->tenant, ciP->servicePathV[0]);
    parseDataP->rcr.res.registrationId.set(fwdRegId);
    mongoSetFwdRegId(rcrP->registrationId.get(), fwdRegId, ciP->tenant);
    std::string payload = parseDataP->rcr.res.render(RegisterContext, ciP->inFormat, "");
    fordwardRegisterContext(fwdHost, fwdPort, ciP->tenant, ciP->httpHeaders.xauthToken, payload, ciP->servicePathV[0]);
  }
}



/* ****************************************************************************
*
* postRegisterContext -
*/
std::string postRegisterContext
(
  ConnectionInfo*            ciP,
  int                        components,
  std::vector<std::string>&  compV,
  ParseData*                 parseDataP
)
{
  RegisterContextResponse  rcr;
  std::string              answer;

  //
  // If more than ONE service-path is input, an error is returned as response.
  // If NO service-path is issued, then the default service-path "/" is used.
  // After these checks, the service-path is checked to be 'correct'.
  //
  if (ciP->servicePathV.size() > 1)
  {
    LM_W(("Bad Input (more than one service path for a registration)"));
    rcr.errorCode.fill(SccBadRequest, "more than one service path for notification");
    answer = rcr.render(RegisterContext, ciP->outFormat, "");
    return answer;
  }
  else if (ciP->servicePathV.size() == 0)
  {
    ciP->servicePathV.push_back(DEFAULT_SERVICE_PATH);
  }

  std::string res = servicePathCheck(ciP->servicePathV[0].c_str());
  if (res != "OK")
  {
    rcr.errorCode.fill(SccBadRequest, res);
    answer = rcr.render(RegisterContext, ciP->outFormat, "");
    return answer;
  }

  if (fwdPort != 0)
  {
    registerContextForward(ciP, parseDataP, &rcr);
  }
  else
  {
    ciP->httpStatusCode = mongoRegisterContext(&parseDataP->rcr.res, &rcr, ciP->uriParam, ciP->tenant, ciP->servicePathV[0]);
  }

  answer = rcr.render(RegisterContext, ciP->outFormat, "");

  return answer;
}
