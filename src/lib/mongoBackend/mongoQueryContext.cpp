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
* Author: Fermin Galan Marquez
*/
#include <string>

#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"

#include "common/sem.h"

#include "mongoBackend/MongoGlobal.h"
#include "mongoBackend/mongoQueryContext.h"

#include "ngsi/ContextRegistrationResponse.h"
#include "ngsi10/QueryContextRequest.h"
#include "ngsi10/QueryContextResponse.h"

/* ****************************************************************************
*
* mongoQueryContext - 
*/
HttpStatusCode mongoQueryContext(QueryContextRequest* requestP, QueryContextResponse* responseP, const std::string& tenant, const std::string& servicePath)
{
    reqSemTake(__FUNCTION__, "ngsi10 query request");

    LM_T(LmtMongo, ("QueryContext Request"));    

    /* FIXME: restriction not supported for the moment */
    if (!requestP->restriction.attributeExpression.isEmpty()) {
        LM_W(("Restriction found but not supported at mongo backend"));
    }

    std::string err;
    if (!entitiesQuery(requestP->entityIdVector, requestP->attributeList, requestP->restriction, &responseP->contextElementResponseVector, &err, true, tenant, servicePath)) {
        responseP->errorCode.fill(SccReceiverInternalError, err);
        LM_E((responseP->errorCode.details.c_str()));
    }
    else if (responseP->contextElementResponseVector.size() == 0) {
      /* If query hasn't any result we have to fill the status code part in the response */
      responseP->errorCode.fill(SccContextElementNotFound);
    }

    reqSemGive(__FUNCTION__, "ngsi10 query request");
    return SccOk;
}
