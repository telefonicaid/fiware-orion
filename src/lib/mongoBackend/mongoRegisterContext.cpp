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
* Author: Fermin Galan Marquez
*/
#include <string>
#include <map>

#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"
#include "common/globals.h"
#include "common/statistics.h"
#include "common/sem.h"
#include "common/defaultValues.h"
#include "alarmMgr/alarmMgr.h"
#include "ngsi/StatusCode.h"
#include "ngsi9/RegisterContextRequest.h"
#include "ngsi9/RegisterContextResponse.h"

#include "mongoBackend/MongoGlobal.h"
#include "mongoBackend/MongoCommonRegister.h"
#include "mongoBackend/dbConstants.h"
#include "mongoBackend/mongoRegisterContext.h"

#include "mongoDriver/safeMongo.h"
#include "mongoDriver/connectionOperations.h"
#include "mongoDriver/BSONObjBuilder.h"



/* ****************************************************************************
*
* mongoRegisterContext - 
*/
HttpStatusCode mongoRegisterContext
(
  RegisterContextRequest*              requestP,
  RegisterContextResponse*             responseP,
  std::map<std::string, std::string>&  uriParam,
  const std::string&                   fiwareCorrelator,
  const std::string&                   tenant,
  const std::string&                   servicePath
)
{
  bool         reqSemTaken;

  // FIXME P4: See issue #3078
  std::string  sPath = (servicePath.empty())? SERVICE_PATH_ROOT : servicePath;

  reqSemTake(__FUNCTION__, "ngsi9 register request", SemWriteOp, &reqSemTaken);

  /* Check if new registration */
  if (requestP->registrationId.isEmpty())
  {
    HttpStatusCode result = processRegisterContext(requestP, responseP, NULL, tenant, sPath, "JSON", fiwareCorrelator);

    reqSemGive(__FUNCTION__, "ngsi9 register request", reqSemTaken);
    return result;
  }

  /* It is not a new registration, so it must be an update */
  orion::BSONObj  reg;
  std::string     err;
  orion::OID      id;

  if (!safeGetRegId(requestP->registrationId, &id, &(responseP->errorCode)))
  {
    reqSemGive(__FUNCTION__, "ngsi9 register request (safeGetRegId fail)", reqSemTaken);
    responseP->registrationId = requestP->registrationId;
    ++noOfRegistrationUpdateErrors;

    if (responseP->errorCode.code == SccContextElementNotFound)
    {
      // FIXME: doubt: invalid OID format?
      std::string details = std::string("invalid OID format: '") + requestP->registrationId.get() + "'";
      alarmMgr.badInput(clientIp, details);
    }
    else  // SccReceiverInternalError
    {
      LM_E(("Runtime Error (exception getting OID: %s)", responseP->errorCode.details.c_str()));
    }

    return SccOk;
  }

  const std::string    colName  = getRegistrationsCollectionName(tenant);

  orion::BSONObjBuilder bob;
  bob.append("_id", id);
  bob.append(REG_SERVICE_PATH, sPath);

  if (!orion::collectionFindOne(colName, bob.obj(), &reg, &err))
  {
    // FIXME OLD-DR: at the present moment we are unable to know if false means: "no result" or
    // "fail in cursor". Asked: https://stackoverflow.com/questions/66027858/how-to-get-errors-when-calling-mongoc-collection-find-function
    // We assume "no result" by the moment, so disabling this code.
    /*reqSemGive(__FUNCTION__, "ngsi9 register request", reqSemTaken);
    responseP->errorCode.fill(SccReceiverInternalError, err);
    ++noOfRegistrationUpdateErrors;

    return SccOk;*/
  }

  if (reg.isEmpty())
  {
    reqSemGive(__FUNCTION__, "ngsi9 register request (no registrations found)", reqSemTaken);
    responseP->errorCode.fill(SccContextElementNotFound,
                              std::string("registration id: /") + requestP->registrationId.get() + "/");
    responseP->registrationId = requestP->registrationId;
    ++noOfRegistrationUpdateErrors;

    return SccOk;
  }

  HttpStatusCode result = processRegisterContext(requestP, responseP, &id, tenant, sPath, "JSON", fiwareCorrelator);
  reqSemGive(__FUNCTION__, "ngsi9 register request", reqSemTaken);
  return result;
}
