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

#include "mongo/client/dbclient.h"

#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"
#include "common/globals.h"
#include "common/statistics.h"
#include "common/sem.h"
#include "common/defaultValues.h"
#include "alarmMgr/alarmMgr.h"

#include "mongoBackend/mongoRegisterContext.h"
#include "mongoBackend/MongoGlobal.h"
#include "mongoBackend/connectionOperations.h"
#include "mongoBackend/MongoCommonRegister.h"
#include "mongoBackend/dbConstants.h"
#include "mongoBackend/safeMongo.h"
#include "ngsi/StatusCode.h"
#include "ngsi9/RegisterContextRequest.h"
#include "ngsi9/RegisterContextResponse.h"

using namespace mongo;



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
    std::string        sPath         = servicePath;    
    bool               reqSemTaken;    

    reqSemTake(__FUNCTION__, "ngsi9 register request", SemWriteOp, &reqSemTaken);

    // Default value for service-path is "/"
    if (sPath == "")
    {
      sPath = DEFAULT_SERVICE_PATH_UPDATES;
    }

    /* Check if new registration */
    if (requestP->registrationId.isEmpty())
    {
      HttpStatusCode result = processRegisterContext(requestP, responseP, NULL, tenant, sPath, "JSON", fiwareCorrelator);
      reqSemGive(__FUNCTION__, "ngsi9 register request", reqSemTaken);
      return result;
    }

    /* It is not a new registration, so it should be an update */
    BSONObj     reg;
    std::string err;
    OID         id;

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
      else // SccReceiverInternalError
      {
        LM_E(("Runtime Error (exception getting OID: %s)", responseP->errorCode.details.c_str()));
      }
      return SccOk;
    }

    if (!collectionFindOne(getRegistrationsCollectionName(tenant).c_str(), BSON("_id" << id << REG_SERVICE_PATH << sPath), &reg, &err))
    {
      reqSemGive(__FUNCTION__, "ngsi9 register request", reqSemTaken);
      responseP->errorCode.fill(SccReceiverInternalError, err);
      ++noOfRegistrationUpdateErrors;
      return SccOk;
    }

    if (reg.isEmpty())
    {
       reqSemGive(__FUNCTION__, "ngsi9 register request (no registrations found)", reqSemTaken);
       responseP->errorCode.fill(SccContextElementNotFound, std::string("registration id: /") + requestP->registrationId.get() + "/");
       responseP->registrationId = requestP->registrationId;
       ++noOfRegistrationUpdateErrors;
       return SccOk;
    }

    HttpStatusCode result = processRegisterContext(requestP, responseP, &id, tenant, sPath, "JSON", fiwareCorrelator);
    reqSemGive(__FUNCTION__, "ngsi9 register request", reqSemTaken);
    return result;
}
