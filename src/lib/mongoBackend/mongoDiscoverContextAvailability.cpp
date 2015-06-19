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
#include <map>
#include <vector>

#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"
#include "common/statistics.h"
#include "common/sem.h"
#include "mongoBackend/mongoDiscoverContextAvailability.h"
#include "rest/HttpStatusCode.h"
#include "mongoBackend/MongoGlobal.h"
#include "ngsi/StatusCode.h"
#include "ngsi9/DiscoverContextAvailabilityRequest.h"
#include "ngsi9/DiscoverContextAvailabilityResponse.h"
#include "mongo/client/dbclient.h"

using namespace mongo;
using std::auto_ptr;


/* ****************************************************************************
*
* associationsQuery -
*/
static bool associationsQuery
(
  EntityIdVector*                  enV,
  AttributeList*                   attrL,
  const std::string&               scope,
  MetadataVector*                  mdV,
  std::string*                     err,
  const std::string&               tenant,
  int                              offset,
  int                              limit,
  bool                             details,
  const std::vector<std::string>&  servicePathV
)
{
  DBClientBase* connection = NULL;

  /* Note that SCOPE_VALUE_ASSOC_SOURCE means that the argument is a target (so we use ASSOC_TARGET_ENT and
   * ASSOC_ATTRS_TARGET in the query), while SCOPE_VALUE_ASSOC_TARGET means that the argument is a source (so we
   * use ASSOC_SOURCE_ENT and ASSOC_ATTRS_source in the query) */
  BSONObjBuilder queryB;

  /* Build query (entity part) */
  BSONArrayBuilder enArray;
  for (unsigned int ix = 0; ix < enV->size(); ++ix)
  {
    enArray.append(BSON(ASSOC_ENT_ID << enV->get(ix)->id << ASSOC_ENT_TYPE << enV->get(ix)->type));
  }

  BSONObj queryEn;
  if (scope == SCOPE_VALUE_ASSOC_SOURCE)
  {
    queryB.append(ASSOC_TARGET_ENT, BSON("$in" << enArray.arr()));
  }
  else  // SCOPE_VALUE_ASSOC_TARGET
  {
    queryB.append(ASSOC_SOURCE_ENT, BSON("$in" << enArray.arr()));
  }

  /* Build query (attribute part) */
  BSONArrayBuilder attrArray;
  for (unsigned int ix = 0; ix < attrL->size() ; ++ix)
  {
    attrArray.append(attrL->get(ix));
  }

  std::string attrField;
  if (scope == SCOPE_VALUE_ASSOC_SOURCE)
  {
    attrField = ASSOC_ATTRS "." ASSOC_ATTRS_TARGET;
  }
  else   // SCOPE_VALUE_ASSOC_TARGET
  {
    attrField = ASSOC_ATTRS "." ASSOC_ATTRS_SOURCE;
  }

  // If there are no attributes specified we want them all
  if (attrArray.arrSize() != 0)
  {
    queryB.append(attrField, BSON("$in" << attrArray.arr()));
  }

  /* Do the query in MongoDB */
  auto_ptr<DBClientCursor> cursor;
  Query                    query(queryB.obj());
  Query                    sortCriteria  = query.sort(BSON("_id" << 1));

  try
  {
    LM_T(LmtMongo, ("query() in '%s' collection: '%s'",
                    getAssociationsCollectionName(tenant).c_str(),
                    query.toString().c_str()));

    connection = getMongoConnection();
    cursor = connection->query(getAssociationsCollectionName(tenant).c_str(), query, limit, offset);

    /*
     * We have observed that in some cases of DB errors (e.g. the database daemon is down) instead of
     * raising an exception, the query() method sets the cursor to NULL. In this case, we raise the
     * exception ourselves
     */
    if (cursor.get() == NULL)
    {
      throw DBException("Null cursor from mongo (details on this is found in the source code)", 0);
    }
    releaseMongoConnection(connection);


    LM_I(("Database Operation Successful (%s)", query.toString().c_str()));
  }
  catch (const DBException &e)
  {
    releaseMongoConnection(connection);

    *err = std::string("collection: ") + getAssociationsCollectionName(tenant).c_str() +
      " - query(): " + query.toString() +
      " - exception: " + e.what();

    LM_E(("Database Error ('%s', '%s')", query.toString().c_str(), err->c_str()));
    return false;
  }
  catch (...)
  {
    releaseMongoConnection(connection);

    *err = std::string("collection: ") + getAssociationsCollectionName(tenant).c_str() +
      " - query(): " + query.toString() +
      " - exception: " + "generic";

    LM_E(("Database Error ('%s', '%s')", query.toString().c_str(), err->c_str()));
    return false;
  }

  /* Process query result */
  while (cursor->more())
  {
    BSONObj      r          = cursor->next();
    std::string  name       = STR_FIELD(r, "_id");
    std::string  srcEnId    = STR_FIELD(r.getField(ASSOC_SOURCE_ENT).embeddedObject(), ASSOC_ENT_ID);
    std::string  srcEnType  = STR_FIELD(r.getField(ASSOC_SOURCE_ENT).embeddedObject(), ASSOC_ENT_TYPE);
    std::string  tgtEnId    = STR_FIELD(r.getField(ASSOC_TARGET_ENT).embeddedObject(), ASSOC_ENT_ID);
    std::string  tgtEnType  = STR_FIELD(r.getField(ASSOC_TARGET_ENT).embeddedObject(), ASSOC_ENT_TYPE);
    Metadata*    md         = new Metadata(name, "Association");

    LM_T(LmtMongo, ("retrieved document: '%s'", r.toString().c_str()));

    md->association.entityAssociation.source.fill(srcEnId, srcEnType, "false");
    md->association.entityAssociation.target.fill(tgtEnId, tgtEnType, "false");

    std::vector<BSONElement> attrs = r.getField(ASSOC_ATTRS).Array();

    for (unsigned int ix = 0; ix < attrs.size(); ++ix)
    {
      std::string            srcAttr    = STR_FIELD(attrs[ix].embeddedObject(), ASSOC_ATTRS_SOURCE);
      std::string            tgtAttr    = STR_FIELD(attrs[ix].embeddedObject(), ASSOC_ATTRS_TARGET);
      AttributeAssociation*  attrAssoc  = new AttributeAssociation();

      attrAssoc->source = srcAttr;
      attrAssoc->target = tgtAttr;

      md->association.attributeAssociationList.push_back(attrAssoc);
    }

    mdV->push_back(md);
  }

  return true;
}


/* ****************************************************************************
*
* associationsDiscoverContextAvailability -
*/
static HttpStatusCode associationsDiscoverContextAvailability
(
  DiscoverContextAvailabilityRequest*   requestP,
  DiscoverContextAvailabilityResponse*  responseP,
  const std::string&                    scope,
  const std::string&                    tenant,
  int                                   offset,
  int                                   limit,
  bool                                  details,
  const std::vector<std::string>&       servicePathV
)
{
  if (scope == SCOPE_VALUE_ASSOC_ALL)
  {
    LM_W(("Bad Input (%s scope not supported)", SCOPE_VALUE_ASSOC_ALL));
    responseP->errorCode.fill(SccNotImplemented, std::string("Not supported scope: '") + SCOPE_VALUE_ASSOC_ALL + "'");
    return SccOk;
  }

  MetadataVector  mdV;
  std::string     err;

  if (!associationsQuery(&requestP->entityIdVector,
                         &requestP->attributeList,
                         scope,
                         &mdV,
                         &err,
                         tenant,
                         offset,
                         limit,
                         details,
                         servicePathV))
  {
    mdV.release();
    responseP->errorCode.fill(SccReceiverInternalError, std::string("Database error: ") + err);
    return SccOk;
  }

  LM_T(LmtPagination, ("Offset: %d, Limit: %d, Details: %s", offset, limit, (details == true)? "true" : "false"));

  /* Query for associated entities */
  for (unsigned int ix = 0; ix < mdV.size(); ++ix)
  {
    /* Each association involves a registrationsQuery() operation, accumulating the answer in
     * responseP->responseVector
     */
    Metadata*       md = mdV.get(ix);
    EntityIdVector  enV;
    AttributeList   attrL;
    EntityId        en;

    if (scope == SCOPE_VALUE_ASSOC_SOURCE)
    {
      en = EntityId(md->association.entityAssociation.source.id, md->association.entityAssociation.source.type);
    }
    else  // SCOPE_VALUE_ASSOC_TARGET
    {
      en = EntityId(md->association.entityAssociation.target.id, md->association.entityAssociation.target.type);
    }
    enV.push_back(&en);

    for (unsigned int jx = 0; jx < md->association.attributeAssociationList.size(); ++jx)
    {
      if (scope == SCOPE_VALUE_ASSOC_SOURCE)
      {
        attrL.push_back(md->association.attributeAssociationList.get(jx)->source);
      }
      else
      {
        attrL.push_back(md->association.attributeAssociationList.get(jx)->target);
      }
    }

    ContextRegistrationResponseVector crrV;
    if (!registrationsQuery(enV, attrL, &crrV, &err, tenant, servicePathV))
    {
      responseP->errorCode.fill(SccReceiverInternalError, err);
      mdV.release();
      return SccOk;
    }

    /* Accumulate in responseP */
    for (unsigned int jx = 0; jx < crrV.size(); ++jx)
    {
      responseP->responseVector.push_back(crrV.get(jx));
    }
  }

  if (responseP->responseVector.size() == 0)
  {
    mdV.release();

    responseP->errorCode.fill(SccContextElementNotFound,
                              "Could not query association with combination of entity/attribute");

    LM_RE(SccOk, (responseP->errorCode.details.c_str()));
  }

  /* Set association metadata as final ContextRegistrationResponse */
  ContextRegistrationResponse* crrMd = new ContextRegistrationResponse();

  crrMd->contextRegistration.providingApplication.set("http://www.fi-ware.eu/NGSI/association");
  crrMd->contextRegistration.registrationMetadataVector = mdV;
  responseP->responseVector.push_back(crrMd);

  return SccOk;
}


/* ****************************************************************************
*
* conventionalDiscoverContextAvailability -
*/
static HttpStatusCode conventionalDiscoverContextAvailability
(
  DiscoverContextAvailabilityRequest*   requestP,
  DiscoverContextAvailabilityResponse*  responseP,
  const std::string&                    tenant,
  int                                   offset,
  int                                   limit,
  bool                                  details,
  const std::vector<std::string>&       servicePathV
)
{
  std::string  err;
  long long    count = -1;

  LM_T(LmtPagination, ("Offset: %d, Limit: %d, Details: %s", offset, limit, (details == true)? "true" : "false"));
  if (!registrationsQuery(requestP->entityIdVector,
                          requestP->attributeList,
                          &responseP->responseVector,
                          &err,
                          tenant,
                          servicePathV,
                          offset,
                          limit,
                          details,
                          &count))
  {
    responseP->errorCode.fill(SccReceiverInternalError, err);
    LM_E(("Database Error (%s)", responseP->errorCode.details.c_str()));
    return SccOk;
  }

  if (responseP->responseVector.size() == 0)
  {
    //
    // If the responseV is empty, we haven't found any entity and have to fill the status code part in the response.
    //
    // However, if the response was empty due to a too high pagination offset,
    // and if the user has asked for 'details' (as URI parameter), then the response should include information about
    // the number of hits without pagination.
    //
    if (details)
    {
      if ((count > 0) && (offset >= count))
      {
        char details[256];

        snprintf(details, sizeof(details), "Number of matching registrations: %lld. Offset is %d", count, offset);
        responseP->errorCode.fill(SccContextElementNotFound, details);
        return SccOk;
      }
    }

    responseP->errorCode.fill(SccContextElementNotFound);
    return SccOk;
  }
  else if (details == true)
  {
    //
    // If all was OK, but the details URI param was set to 'on', then the responses error code details
    // 'must' contain the total count of hits.
    //

    char details[64];

    snprintf(details, sizeof(details), "Count: %lld", count);
    responseP->errorCode.fill(SccOk, details);
  }

  return SccOk;
}


/* ****************************************************************************
*
* mongoDiscoverContextAvailability -
*/
HttpStatusCode mongoDiscoverContextAvailability
(
  DiscoverContextAvailabilityRequest*   requestP,
  DiscoverContextAvailabilityResponse*  responseP,
  const std::string&                    tenant,
  std::map<std::string, std::string>&   uriParams,
  const std::vector<std::string>&       servicePathV
)
{
  int          offset         = atoi(uriParams[URI_PARAM_PAGINATION_OFFSET].c_str());
  int          limit          = atoi(uriParams[URI_PARAM_PAGINATION_LIMIT].c_str());
  std::string  detailsString  = uriParams[URI_PARAM_PAGINATION_DETAILS];
  bool         details        = (strcasecmp("on", detailsString.c_str()) == 0)? true : false;
  bool         reqSemTaken;

  reqSemTake(__FUNCTION__, "mongo ngsi9 discovery request", SemReadOp, &reqSemTaken);

  LM_T(LmtMongo, ("DiscoverContextAvailability Request"));

  /* Depending on the scope used, we invoke one function or another. DiscoverContextAvailability may behave
   * differently depending on the scope. Although OperationScope is a list in NGSI, we only support ONE
   * scope at a time */
  int nScopes = requestP->restriction.scopeVector.size();
  if (nScopes > 0)
  {
    if (nScopes > 1)
    {
      LM_W(("Bad Input (%d scopes: only the first one is used)", nScopes));
    }

    std::string scopeType  = requestP->restriction.scopeVector.get(0)->type;
    std::string scopeValue = requestP->restriction.scopeVector.get(0)->value;

    if (scopeType == SCOPE_TYPE_ASSOC)
    {
      HttpStatusCode ms = associationsDiscoverContextAvailability(requestP,
                                                                  responseP,
                                                                  scopeValue,
                                                                  tenant,
                                                                  offset,
                                                                  limit,
                                                                  details,
                                                                  servicePathV);

      reqSemGive(__FUNCTION__, "mongo ngsi9 discovery request (association)");
      return ms;
    }
    else
    {
      LM_W(("Bad Input (unsupported scope [%s, %s], doing conventional discoverContextAvailability)",
            scopeType.c_str(),
            scopeValue.c_str()));
    }
  }

  HttpStatusCode hsCode = conventionalDiscoverContextAvailability(requestP,
                                                                  responseP,
                                                                  tenant,
                                                                  offset,
                                                                  limit,
                                                                  details,
                                                                  servicePathV);
  if (hsCode != SccOk)
  {
    ++noOfDiscoveryErrors;
  }

  reqSemGive(__FUNCTION__, "mongo ngsi9 discovery request", reqSemTaken);

  return hsCode;
}
