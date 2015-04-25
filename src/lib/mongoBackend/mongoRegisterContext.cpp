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

#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"
#include "common/globals.h"
#include "common/statistics.h"
#include "common/sem.h"
#include "common/defaultValues.h"
#include "mongoBackend/mongoRegisterContext.h"
#include "mongoBackend/MongoGlobal.h"
#include "mongoBackend/MongoCommonRegister.h"
#include "ngsi/StatusCode.h"
#include "ngsi9/RegisterContextRequest.h"
#include "ngsi9/RegisterContextResponse.h"

#include "mongo/client/dbclient.h"

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
  const std::string&                   tenant,
  const std::string&                   servicePath
)
{
    std::string  sPath         = servicePath;
    std::string  notifyFormat  = uriParam[URI_PARAM_NOTIFY_FORMAT];

    LM_T(LmtMongo, ("Register Context Request: '%s' format", notifyFormat.c_str()));

    reqSemTake(__FUNCTION__, "ngsi9 register request");    

    DBClientBase* connection = getMongoConnection();

    // Default value for service-path is "/"
    if (sPath == "")
    {
      sPath = DEFAULT_SERVICE_PATH;
    }

    /* Check if new registration */
    if (requestP->registrationId.isEmpty())
    {
      HttpStatusCode result = processRegisterContext(requestP, responseP, NULL, tenant, sPath, notifyFormat);
      reqSemGive(__FUNCTION__, "ngsi9 register request");
      return result;
    }

    /* It is not a new registration, so it should be an update */
    BSONObj reg;
    OID     id;
    try
    {
        id = OID(requestP->registrationId.get());

        mongoSemTake(__FUNCTION__, "findOne from RegistrationsCollection");
        reg = connection->findOne(getRegistrationsCollectionName(tenant).c_str(), BSON("_id" << id << "servicePath" << sPath));
        mongoSemGive(__FUNCTION__, "findOne from RegistrationsCollection");

        LM_I(("Database Operation Successful (findOne _id: %s)", id.toString().c_str()));
    }
    catch (const AssertionException &e)
    {
        mongoSemGive(__FUNCTION__, "findOne from RegistrationsCollection (AssertionException)");
        reqSemGive(__FUNCTION__, "ngsi9 register request");

        /* This happens when OID format is wrong */
        // FIXME: this checking should be done at parsing stage, without progressing to
        // mongoBackend. By the moment we can live this here, but we should remove in the future
        responseP->errorCode.fill(SccContextElementNotFound);
        responseP->registrationId = requestP->registrationId;
        ++noOfRegistrationUpdateErrors;
        LM_W(("Bad Input (invalid OID format)"));
        return SccOk;
    }
    catch (const DBException &e)
    {
        mongoSemGive(__FUNCTION__, "findOne from RegistrationsCollection (DBException)");
        reqSemGive(__FUNCTION__, "ngsi9 register request");

        responseP->errorCode.fill(SccReceiverInternalError,
                                  std::string("collection: ") + getRegistrationsCollectionName(tenant).c_str() +
                                  " - findOne() _id: " + requestP->registrationId.get() +
                                  " - exception: " + e.what());
        ++noOfRegistrationUpdateErrors;
        LM_E(("Database Error (%s)", responseP->errorCode.details.c_str()));
        return SccOk;
    }
    catch (...)
    {
        mongoSemGive(__FUNCTION__, "findOne from RegistrationsCollection (Generic Exception)");
        reqSemGive(__FUNCTION__, "ngsi9 register request");

        responseP->errorCode.fill(SccReceiverInternalError,
                                  std::string("collection: ") + getRegistrationsCollectionName(tenant).c_str() +
                                  " - findOne() _id: " + requestP->registrationId.get() +
                                  " - exception: " + "generic");
        ++noOfRegistrationUpdateErrors;
        LM_E(("Database Error (%s)", responseP->errorCode.details.c_str()));
        return SccOk;
    }

    if (reg.isEmpty())
    {
       reqSemGive(__FUNCTION__, "ngsi9 register request (no registrations found)");
       responseP->errorCode.fill(SccContextElementNotFound, std::string("registration id: /") + requestP->registrationId.get() + "/");
       responseP->registrationId = requestP->registrationId;
       ++noOfRegistrationUpdateErrors;
       return SccOk;
    }

    HttpStatusCode result = processRegisterContext(requestP, responseP, &id, tenant, sPath, notifyFormat);
    reqSemGive(__FUNCTION__, "ngsi9 register request");
    return result;
}
