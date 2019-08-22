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
#include "orionld/context/orionldAliasLookup.h"                // orionldAliasLookup
#include "orionld/context/orionldUriExpand.h"                  // orionldUriExpand
#include "mongoBackend/MongoGlobal.h"                          // getMongoConnection
#include "mongoBackend/safeMongo.h"                            // moreSafe
#include "mongoBackend/connectionOperations.h"                 // collectionRangedQuery
#include "mongoBackend/mongoRegistrationAux.h"                 // mongoSetXxx
#include "mongoBackend/mongoLdRegistrationAux.h"               // mongoSetLdRelationshipV, mongoSetLdPropertyV, ...
#include "mongoBackend/mongoLdRegistrationsGet.h"              // Own interface



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
  bool      reqSemTaken = false;
  int       offset      = atoi(ciP->uriParam[URI_PARAM_PAGINATION_OFFSET].c_str());
  int       limit;

  if (ciP->uriParam[URI_PARAM_PAGINATION_LIMIT] != "")
  {
    limit = atoi(ciP->uriParam[URI_PARAM_PAGINATION_LIMIT].c_str());
    if (limit <= 0)
      limit = DEFAULT_PAGINATION_LIMIT_INT;
  }
  else
    limit = DEFAULT_PAGINATION_LIMIT_INT;

  LM_T(LmtMongo, ("Mongo GET Registrations"));

  /* ONTIMEINTERVAL Registrations are not part of NGSIv2, so they are excluded.
   * Note that expiration is not taken into account (in the future, a q= query
   * could be added to the operation in order to filter results)
   */
  std::auto_ptr<mongo::DBClientCursor>  cursor;
  std::string                           err;
  mongo::BSONObjBuilder                 queryBuilder;
  mongo::Query                          query;

  //
  // FIXME: This function will grow too long - need to create a function for each URI param
  //        and move all of it out to a separate file: mongoLdRegistrationsGet.cpp
  //
  //
  // The function should do something like this:
  //
  // const char*     uriParamId   = ciP->uriParam["id"].c_str();
  // const char*     uriParamType = ciP->uriParam["type"].c_str();
  //
  // if (uriParamId != NULL)
  //   uriParamIdToFilter(&queryBuilder, uriParamId);
  //
  // if (uriParamType != NULL)
  //   uriParamTypeToFilter(&queryBuilder, uriParamType);
  //
  // ... More calls to add uri params to filter ...
  //
  // query = queryBuilder.obj();
  //
  //
  if (ciP->uriParam["id"] != "")
  {
    char*                       idList = (char*) ciP->uriParam["id"].c_str();
    std::vector<std::string>    idVec;
    int                         ids;
    mongo::BSONObjBuilder       bsonInExpression;
    mongo::BSONArrayBuilder     bsonArray;

    ids = stringSplit(idList, ',', idVec);

    if (ids > 0)
    {
      for (int ix = 0; ix < ids; ix++)
        bsonArray.append(idVec[ix]);

      bsonInExpression.append("$in", bsonArray.arr());
      queryBuilder.append("_id", bsonInExpression.obj());
    }
  }

  if (ciP->uriParam["type"] != "")
  {
    char*                       typeList = (char*) ciP->uriParam["type"].c_str();
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

    if (types > 0)
    {
      char typeExpanded[256];

      for (int ix = 0; ix < types; ix++)
      {
        char* type = (char*) typeVec[ix].c_str();

        if (((strncmp(type, "http://", 7) == 0) || (strncmp(type, "https://", 8) == 0)) && (urlCheck(type, &details) == true))
        {
          // No expansion desired, the type is already a FQN
          bsonArray.append(type);
        }
        else
        {
          if (orionldUriExpand(orionldState.contextP, type, typeExpanded, sizeof(typeExpanded), &details) == false)
          {
            orionldErrorResponseCreate(OrionldBadRequestData, "Error during URI expansion of entity type", details, OrionldDetailsString);
            return false;
          }

          bsonArray.append(typeExpanded);
        }
      }

      bsonInExpression.append("$in", bsonArray.arr());
      queryBuilder.append("contextRegistration.entities.type", bsonInExpression.obj());
    }
  }

  //
  // FIXME: Many more URI params to be treated and added to queryBuilder
  //

  query = queryBuilder.obj();
  query.sort(BSON("_id" << 1));
  // LM_TMP(("KZ: query: %s", query.toString().c_str()));
  TIME_STAT_MONGO_READ_WAIT_START();
  reqSemTake(__FUNCTION__, "Mongo GET Registrations", SemReadOp, &reqSemTaken);

  mongo::DBClientBase* connection = getMongoConnection();

  if (!collectionRangedQuery(connection,
                             getRegistrationsCollectionName(tenant),
                             query,
                             limit,
                             offset,
                             &cursor,
                             countP,
                             &err))
  {
    LM_TMP(("INSIDE"));

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

    if (!nextSafeOrErrorF(cursor, &bob, &err))
    {
      LM_E(("Runtime Error (exception in nextSafe(): %s - query: %s)", err.c_str(), query.toString().c_str()));
      continue;
    }
    docs++;
    LM_T(LmtMongo, ("retrieved document %d: '%s'", docs, bob.toString().c_str()));

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

    regVecP->push_back(reg);
  }

  releaseMongoConnection(connection);
  reqSemGive(__FUNCTION__, "Mongo List Registrations", reqSemTaken);

  oeP->code = SccOk;
  return true;
}
