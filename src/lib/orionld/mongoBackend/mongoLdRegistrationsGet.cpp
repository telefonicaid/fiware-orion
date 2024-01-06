/*
*
* Copyright 2019 FIWARE Foundation e.V.
*
* This file is part of Orion-LD Context Broker.
*
* Orion-LD Context Broker is free software: you can redistribute it and/or
* modify it under the terms of the GNU Affero General Public License as
* published by the Free Software Foundation, either version 3 of the
* License, or (at your option) any later version.
*
* Orion-LD Context Broker is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero
* General Public License for more details.
*
* You should have received a copy of the GNU Affero General Public License
* along with Orion-LD Context Broker. If not, see http://www.gnu.org/licenses/.
*
* For those usages not covered by this license please contact with
* orionld at fiware dot org
*
* Author: Ken Zangelin, Larysse Savanna and Gabriel Quaresma
*/
#include <string>                                                // std::string
#include <vector>                                                // std::vector

#include "logMsg/logMsg.h"                                       // LM_*
#include "logMsg/traceLevels.h"                                  // Lmt*

#include "orionld/types/OrionldTenant.h"                         // OrionldTenant

#include "common/statistics.h"                                   // TIME_STAT_MONGO_READ_WAIT_START, ...
#include "rest/OrionError.h"                                     // OrionError
#include "apiTypesV2/Registration.h"                             // ngsiv2::Registration
#include "mongoBackend/MongoGlobal.h"                            // getMongoConnection
#include "mongoBackend/safeMongo.h"                              // moreSafe
#include "mongoBackend/connectionOperations.h"                   // collectionRangedQuery
#include "mongoBackend/mongoRegistrationAux.h"                   // mongoSetXxx

#include "orionld/common/orionldState.h"                         // orionldState
#include "orionld/common/orionldError.h"                         // orionldError
#include "orionld/context/orionldContextItemExpand.h"            // orionldContextItemExpand
#include "orionld/mongoBackend/mongoLdRegistrationAux.h"         // mongoSetLdRelationshipV, mongoSetLdPropertyV, ...
#include "orionld/mongoBackend/mongoLdRegistrationsGet.h"        // Own interface



// -----------------------------------------------------------------------------
//
// uriParamIdToFilter -
//
// It is the responsibility of the caller to make sure that 'idList' is non-NULL
//
static void uriParamIdToFilter(mongo::BSONObjBuilder* queryBuilderP, char** idList, int ids)
{
  mongo::BSONObjBuilder       bsonInExpression;
  mongo::BSONArrayBuilder     bsonArray;

  for (int ix = 0; ix < ids; ix++)
  {
    bsonArray.append(idList[ix]);
  }

  bsonInExpression.append("$in", bsonArray.arr());
  queryBuilderP->append("_id", bsonInExpression.obj());
}



// -----------------------------------------------------------------------------
//
// uriParamIdPatternToFilter - regular expression that shall be matched by entity ids satisfying the query
//
// It is the responsibility of the caller to make sure that 'idPattern' is non-NULL
//
static bool uriParamIdPatternToFilter(mongo::BSONObjBuilder* queryBuilderP, char* idPattern, std::string* detailsP)
{
  mongo::BSONObjBuilder  bsonInExpression;

  if (idPattern[0] == 0)
  {
    *detailsP = "URI Param /idPattern/ is empty";
    orionldError(OrionldBadRequestData, "No value for URI Parameter", "idPattern", 400);
    return false;
  }

  bsonInExpression.append("$regex", idPattern);
  queryBuilderP->append("_id", bsonInExpression.obj());
  orionldState.uriParams.idPattern = NULL;

  return true;
}



// -----------------------------------------------------------------------------
//
// uriParamTypeToFilter -
//
// It is the responsibility of the caller to make sure that 'typeList' is non-NULL
//
static void uriParamTypeToFilter(mongo::BSONObjBuilder* queryBuilderP, char** typeList, int types)
{
  mongo::BSONObjBuilder       bsonInExpression;
  mongo::BSONArrayBuilder     bsonArray;

  for (int ix = 0; ix < types; ix++)
  {
    bsonArray.append(typeList[ix]);
  }

  bsonInExpression.append("$in", bsonArray.arr());
  queryBuilderP->append("contextRegistration.entities.type", bsonInExpression.obj());
}



// -----------------------------------------------------------------------------
//
// uriParamAttrsToFilter - List of Attributes (Properties or Relationships) to be retrieved
//
// It is the responsibility of the caller to make sure that 'attrList' is non-NULL
//
static bool uriParamAttrsToFilter(mongo::BSONObjBuilder* queryBuilderP, std::string* detailsP)
{
  mongo::BSONObjBuilder       bsonInExpression;
  mongo::BSONArrayBuilder     bsonArray;

  for (int ix = 0; ix < orionldState.in.attrList.items; ix++)
  {
    bsonArray.append(orionldState.in.attrList.array[ix]);
  }

  bsonInExpression.append("$in", bsonArray.arr());
  queryBuilderP->append("contextRegistration.attrs.name", bsonInExpression.obj());

  return true;
}



/* ****************************************************************************
*
* mongoLdRegistrationsGet -
*/
bool mongoLdRegistrationsGet
(
  std::vector<ngsiv2::Registration>*  regVecP,
  OrionldTenant*                      tenantP,
  long long*                          countP,
  OrionError*                         oeP
)
{
  bool                   reqSemTaken        = false;
  std::string            err;
  mongo::BSONObjBuilder  queryBuilder;
  mongo::Query           query;

  if (orionldState.in.idList.items   > 0) uriParamIdToFilter(&queryBuilder,   orionldState.in.idList.array,   orionldState.in.idList.items);
  if (orionldState.in.typeList.items > 0) uriParamTypeToFilter(&queryBuilder, orionldState.in.typeList.array, orionldState.in.typeList.items);

  if ((orionldState.uriParams.idPattern != NULL) && (uriParamIdPatternToFilter(&queryBuilder, orionldState.uriParams.idPattern, &oeP->details) == false))
    return false;

  if ((orionldState.in.attrList.items > 0) && (uriParamAttrsToFilter(&queryBuilder, &oeP->details) == false))
    return false;


  //
  // FIXME: Many more URI params to be treated and added to queryBuilder - but that's for 2020 ...
  //

  query = queryBuilder.obj();  // Here all the filters added to queryBuilder are "merged" into 'query'
  query.sort(BSON("_id" << 1));

  TIME_STAT_MONGO_READ_WAIT_START();
  reqSemTake(__FUNCTION__, "Mongo GET Registrations", SemReadOp, &reqSemTaken);

  std::auto_ptr<mongo::DBClientCursor>  cursor;
  mongo::DBClientBase*                  connection = getMongoConnection();

  if (!collectionRangedQuery(connection,
                             tenantP->registrations,
                             query,
                             orionldState.uriParams.limit,
                             orionldState.uriParams.offset,
                             &cursor,
                             countP,
                             &err))
  {
    releaseMongoConnection(connection);
    TIME_STAT_MONGO_READ_WAIT_STOP();
    reqSemGive(__FUNCTION__, "Mongo List Registrations", reqSemTaken);

    oeP->code    = SccReceiverInternalError;
    oeP->details = err;
    return false;
  }
  TIME_STAT_MONGO_READ_WAIT_STOP();

  /* Process query result */
  unsigned int docs = 0;

  while (moreSafe(cursor))
  {
    mongo::BSONObj         bob;
    ngsiv2::Registration   reg;
    char*                  title;
    char*                  detail;

    if (!nextSafeOrErrorF(cursor, &bob, &err))
    {
      LM_E(("Runtime Error (exception in nextSafe(): %s - query: %s)", err.c_str(), query.toString().c_str()));
      continue;
    }
    docs++;

    //
    // FIXME: All these calls to mongoSet-functions is also done in mongoLdRegistrationGet().
    //        An aux function should be created for this to avoid the copied code.
    //

    mongoSetLdRegistrationId(&reg, &bob);
    mongoSetLdName(&reg, &bob);
    mongoSetDescription(&reg, &bob);

    if (mongoSetDataProvided(&reg, &bob, false) == false)
    {
      releaseMongoConnection(connection);
      LM_W(("Bad Input (getting registrations with more than one CR is not yet implemented, see issue 3044)"));
      reqSemGive(__FUNCTION__, "Mongo Get Registration", reqSemTaken);
      oeP->code = SccReceiverInternalError;
      return false;
    }

    mongoSetLdObservationInterval(&reg, bob);
    mongoSetLdManagementInterval(&reg, bob);
    mongoSetExpires(&reg, bob);
    mongoSetStatus(&reg, &bob);

    mongoSetLdTimestamp(&reg.createdAt, "createdAt", bob);
    mongoSetLdTimestamp(&reg.modifiedAt, "modifiedAt", bob);

    if (mongoSetLdTimeInterval(&reg.location, "location", bob, &title, &detail) == false)
    {
      LM_E(("Internal Error (mongoSetLdTimeInterval: %s: %s)", title, detail));
      releaseMongoConnection(connection);
      reqSemGive(__FUNCTION__, "Mongo Get Registration", reqSemTaken);
      orionldState.httpStatusCode = 500;
      return false;
    }

    if (mongoSetLdTimeInterval(&reg.observationSpace, "observationSpace", bob, &title, &detail) == false)
    {
      LM_E(("Internal Error (mongoSetLdTimeInterval: %s: %s)", title, detail));
      releaseMongoConnection(connection);
      reqSemGive(__FUNCTION__, "Mongo Get Registration", reqSemTaken);
      orionldState.httpStatusCode = 500;
      return false;
    }

    if (mongoSetLdTimeInterval(&reg.operationSpace, "operationSpace", bob, &title, &detail) == false)
    {
      LM_E(("Internal Error (mongoSetLdTimeInterval: %s: %s)", title, detail));
      releaseMongoConnection(connection);
      reqSemGive(__FUNCTION__, "Mongo Get Registration", reqSemTaken);
      orionldState.httpStatusCode = 500;
      return false;
    }

    if (mongoSetLdProperties(&reg, "properties", bob, &title, &detail) == false)
    {
      LM_E(("Internal Error (mongoSetLdProperties: %s: %s)", title, detail));
      releaseMongoConnection(connection);
      reqSemGive(__FUNCTION__, "Mongo Get Registration", reqSemTaken);
      orionldState.httpStatusCode = 500;
      return false;
    }

    regVecP->push_back(reg);
  }

  releaseMongoConnection(connection);
  reqSemGive(__FUNCTION__, "Mongo List Registrations", reqSemTaken);

  oeP->code = SccOk;
  return true;
}
