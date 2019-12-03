/*
*
* Copyright 2019 Telefonica Investigacion y Desarrollo, S.A.U
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
* Author: Ken Zangelin, Larysse Savanna and Gabriel Quaresma
*/
#include <string>                                              // std::string
#include <vector>                                              // std::vector

#include "common/string.h"                                     // stringSplit
#include "common/statistics.h"                                 // TIME_STAT_MONGO_READ_WAIT_START, ...
#include "rest/ConnectionInfo.h"                               // ConnectionInfo
#include "rest/OrionError.h"                                   // OrionError
#include "apiTypesV2/Registration.h"                           // ngsiv2::Registration
#include "rest/uriParamNames.h"                                // URI_PARAM_PAGINATION_OFFSET, ...
#include "orionld/common/orionldState.h"                       // orionldState
#include "orionld/common/urlCheck.h"                           // urlCheck
#include "orionld/common/orionldErrorResponse.h"               // orionldErrorResponseCreate
#include "orionld/context/orionldContextItemExpand.h"          // orionldContextItemExpand
#include "mongoBackend/MongoGlobal.h"                          // getMongoConnection
#include "mongoBackend/safeMongo.h"                            // moreSafe
#include "mongoBackend/connectionOperations.h"                 // collectionRangedQuery
#include "mongoBackend/mongoRegistrationAux.h"                 // mongoSetXxx
#include "mongoBackend/mongoLdRegistrationAux.h"               // mongoSetLdRelationshipV, mongoSetLdPropertyV, ...
#include "mongoBackend/mongoLdRegistrationsGet.h"              // Own interface



// -----------------------------------------------------------------------------
//
// uriParamIdToFilter -
//
// It is the responsibility of the caller to make sure that 'idList' is non-NULL
//
static bool uriParamIdToFilter(mongo::BSONObjBuilder* queryBuilderP, char* idList, std::string* detailsP)
{
  std::vector<std::string>    idVec;
  int                         ids;
  mongo::BSONObjBuilder       bsonInExpression;
  mongo::BSONArrayBuilder     bsonArray;

  ids = stringSplit(idList, ',', idVec);

  if (ids == 0)
  {
    *detailsP = "URI Param /id/ is empty";
    orionldErrorResponseCreate(OrionldBadRequestData, "No value for URI Parameter", "id");
    return false;
  }

  for (int ix = 0; ix < ids; ix++)
  {
    bsonArray.append(idVec[ix]);
  }

  bsonInExpression.append("$in", bsonArray.arr());
  queryBuilderP->append("_id", bsonInExpression.obj());
  orionldState.uriParams.id = NULL;

  return true;
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
    orionldErrorResponseCreate(OrionldBadRequestData, "No value for URI Parameter", "idPattern");
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
static bool uriParamTypeToFilter(mongo::BSONObjBuilder* queryBuilderP, char* typeList, std::string* detailsP)
{
  std::vector<std::string>    typeVec;
  int                         types;
  mongo::BSONObjBuilder       bsonInExpression;
  mongo::BSONArrayBuilder     bsonArray;
  char*                       details;

  //
  // FIXME: Need a new implementation of stringSplit -
  //        one that doesn't use std::string nor std::vector and that doesn't copy any strings,
  //        only points to them.
  //
  types = stringSplit(typeList, ',', typeVec);

  if (types == 0)
  {
    *detailsP = "URI Param /type/ is empty";
    orionldErrorResponseCreate(OrionldBadRequestData, "No value for URI Parameter", "type");
    return false;
  }

  for (int ix = 0; ix < types; ix++)
  {
    char* type = (char*) typeVec[ix].c_str();

    if ((strncmp(type, "http", 4) == 0) && (urlCheck(type, &details) == true))
    {
      // No expansion desired, the type is already a FQN
      bsonArray.append(type);
    }
    else
    {
      char* typeExpanded = orionldContextItemExpand(orionldState.contextP, type, NULL, true, NULL);
      bsonArray.append(typeExpanded);
    }
  }

  bsonInExpression.append("$in", bsonArray.arr());
  queryBuilderP->append("contextRegistration.entities.type", bsonInExpression.obj());
  orionldState.uriParams.type = NULL;

  return true;
}



// -----------------------------------------------------------------------------
//
// uriParamAttrsToFilter - List of Attributes (Properties or Relationships) to be retrieved
//
// It is the responsibility of the caller to make sure that 'attrsList' is non-NULL
//
static bool uriParamAttrsToFilter(mongo::BSONObjBuilder* queryBuilderP, char* attrsList, std::string* detailsP)
{
  std::vector<std::string>    attrsVec;
  int                         attrs;
  mongo::BSONObjBuilder       bsonInExpression;
  mongo::BSONArrayBuilder     bsonArray;
  char*                       details;

  attrs = stringSplit(attrsList, ',', attrsVec);

  if (attrs == 0)
  {
    *detailsP = "URI Param /attrs/ is empty";
    orionldErrorResponseCreate(OrionldBadRequestData, "No value for URI Parameter", "attrs");
    return false;
  }

  for (int ix = 0; ix < attrs; ix++)
  {
    char* attr = (char*) attrsVec[ix].c_str();

    if ((strncmp(attr, "http", 4) == 0) && (urlCheck(attr, &details) == true))
    {
      // No expansion desired, the attr is already a FQN
      bsonArray.append(attr);
    }
    else
    {
      char* attrExpanded = orionldContextItemExpand(orionldState.contextP, attr, NULL, true, NULL);
      bsonArray.append(attrExpanded);
    }
  }

  bsonInExpression.append("$in", bsonArray.arr());
  queryBuilderP->append("contextRegistration.attrs.name", bsonInExpression.obj());
  orionldState.uriParams.attrs = NULL;

  return true;
}



/* ****************************************************************************
*
* mongoLdRegistrationsGet -
*/
bool mongoLdRegistrationsGet
(
  ConnectionInfo*                     ciP,
  std::vector<ngsiv2::Registration>*  regVecP,
  const char*                         tenant,
  long long*                          countP,
  OrionError*                         oeP
)
{
  bool                   reqSemTaken        = false;
  int                    offset             = 0;
  int                    limit              = DEFAULT_PAGINATION_LIMIT_INT;
  std::string            err;
  mongo::BSONObjBuilder  queryBuilder;
  mongo::Query           query;

  if (ciP->uriParam[URI_PARAM_PAGINATION_LIMIT] != "")
    limit = atoi(ciP->uriParam[URI_PARAM_PAGINATION_LIMIT].c_str());  // Error handling already done by uriArgumentGet() in rest.cpp

  if (ciP->uriParam[URI_PARAM_PAGINATION_OFFSET] != "")
    offset = atoi(ciP->uriParam[URI_PARAM_PAGINATION_OFFSET].c_str());

  if ((orionldState.uriParams.id != NULL) && uriParamIdToFilter(&queryBuilder, orionldState.uriParams.id, &oeP->details) == false)
    return false;

  if ((orionldState.uriParams.type != NULL) && (uriParamTypeToFilter(&queryBuilder, orionldState.uriParams.type, &oeP->details) == false))
    return false;

  if ((orionldState.uriParams.idPattern != NULL) && (uriParamIdPatternToFilter(&queryBuilder, orionldState.uriParams.idPattern, &oeP->details) == false))
    return false;

  if ((orionldState.uriParams.attrs != NULL) && (uriParamAttrsToFilter(&queryBuilder, orionldState.uriParams.attrs, &oeP->details) == false))
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
                             getRegistrationsCollectionName(tenant),
                             query,
                             limit,
                             offset,
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
    LM_T(LmtMongo, ("retrieved document %d: '%s'", docs, bob.toString().c_str()));

    //
    // FIXME: All these calls to mongoSet-functions is also done in mongoLdRegistrationGet().
    //        An aux function should be created for this to avoid the copied code.
    //

    mongoSetLdRegistrationId(&reg, bob);
    mongoSetLdName(&reg, bob);
    mongoSetDescription(&reg, bob);

    if (mongoSetDataProvided(&reg, bob, false) == false)
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
    mongoSetStatus(&reg, bob);

    mongoSetLdTimestamp(&reg.createdAt, "createdAt", bob);
    mongoSetLdTimestamp(&reg.modifiedAt, "modifiedAt", bob);

    if (mongoSetLdTimeInterval(&reg.location, "location", bob, &title, &detail) == false)
    {
      LM_E(("Internal Error (mongoSetLdTimeInterval: %s: %s)", title, detail));
      releaseMongoConnection(connection);
      reqSemGive(__FUNCTION__, "Mongo Get Registration", reqSemTaken);
      ciP->httpStatusCode = SccReceiverInternalError;
      return false;
    }

    if (mongoSetLdTimeInterval(&reg.observationSpace, "observationSpace", bob, &title, &detail) == false)
    {
      LM_E(("Internal Error (mongoSetLdTimeInterval: %s: %s)", title, detail));
      releaseMongoConnection(connection);
      reqSemGive(__FUNCTION__, "Mongo Get Registration", reqSemTaken);
      ciP->httpStatusCode = SccReceiverInternalError;
      return false;
    }

    if (mongoSetLdTimeInterval(&reg.operationSpace, "operationSpace", bob, &title, &detail) == false)
    {
      LM_E(("Internal Error (mongoSetLdTimeInterval: %s: %s)", title, detail));
      releaseMongoConnection(connection);
      reqSemGive(__FUNCTION__, "Mongo Get Registration", reqSemTaken);
      ciP->httpStatusCode = SccReceiverInternalError;
      return false;
    }

    if (mongoSetLdProperties(&reg, "properties", bob, &title, &detail) == false)
    {
      LM_E(("Internal Error (mongoSetLdProperties: %s: %s)", title, detail));
      releaseMongoConnection(connection);
      reqSemGive(__FUNCTION__, "Mongo Get Registration", reqSemTaken);
      ciP->httpStatusCode = SccReceiverInternalError;
      return false;
    }

    regVecP->push_back(reg);
  }

  releaseMongoConnection(connection);
  reqSemGive(__FUNCTION__, "Mongo List Registrations", reqSemTaken);

  oeP->code = SccOk;
  return true;
}
