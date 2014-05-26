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

/* ****************************************************************************
*
* associationsQuery -
*/
bool associationsQuery(EntityIdVector enV, AttributeList attrL, std::string scope, MetadataVector* mdV, std::string* err, std::string tenant) {

    DBClientConnection* connection = getMongoConnection();

    /* Note that SCOPE_VALUE_ASSOC_SOURCE means that the argument is a target (so we use ASSOC_TARGET_ENT and
     * ASSOC_ATTRS_TARGET in the query), while SCOPE_VALUE_ASSOC_TARGET means that the argument is a source (so we
     * use ASSOC_SOURCE_ENT and ASSOC_ATTRS_source in the query) */
    BSONObjBuilder queryB;

    /* Build query (entity part) */
    BSONArrayBuilder enArray;
    for (unsigned int ix = 0; ix < enV.size() ; ++ix) {
        enArray.append(BSON(ASSOC_ENT_ID << enV.get(ix)->id << ASSOC_ENT_TYPE << enV.get(ix)->type));
    }
    BSONObj queryEn;
    if (scope == SCOPE_VALUE_ASSOC_SOURCE) {
        queryB.append(ASSOC_TARGET_ENT, BSON("$in" << enArray.arr()));

    }
    else {  // SCOPE_VALUE_ASSOC_TARGET
        queryB.append(ASSOC_SOURCE_ENT, BSON("$in" << enArray.arr()));
    }

    /* Build query (attribute part) */
    BSONArrayBuilder attrArray;
    for (unsigned int ix = 0; ix < attrL.size() ; ++ix) {
        attrArray.append(attrL.get(ix));
    }
    std::string attrField;
    if (scope == SCOPE_VALUE_ASSOC_SOURCE) {
        attrField = ASSOC_ATTRS "." ASSOC_ATTRS_TARGET;
    }
    else {  // SCOPE_VALUE_ASSOC_TARGET
        attrField = ASSOC_ATTRS "." ASSOC_ATTRS_SOURCE;
    }
    queryB.append(attrField, BSON("$in" << attrArray.arr()));

    /* Do query in MongoDB */
    BSONObj query = queryB.obj();
    auto_ptr<DBClientCursor> cursor;
    try {
        LM_T(LmtMongo, ("query() in '%s' collection: '%s'", getAssociationsCollectionName(tenant).c_str(), query.toString().c_str()));

        mongoSemTake(__FUNCTION__, "query in AssociationsCollection");
        cursor = connection->query(getAssociationsCollectionName(tenant).c_str(), query);

        /*
         * We have observed that in some cases of DB errors (e.g. the database daemon is down) instead of
         * raising an exception, the query() method sets the cursor to NULL. In this case, we raise the
         * exception ourselves
         */
        if (cursor.get() == NULL) {
            throw DBException("Null cursor from mongo (details on this is found in the source code)", 0);
        }
        mongoSemGive(__FUNCTION__, "query in AssociationsCollection");
    }
    catch( const DBException &e ) {

        mongoSemGive(__FUNCTION__, "query in AssociationsCollection (DBException)");
        *err = std::string("collection: ") + getAssociationsCollectionName(tenant).c_str() +
                " - query(): " + query.toString() +
                " - exception: " + e.what();
        return false;
    }
    catch(...) {

        mongoSemGive(__FUNCTION__, "query in AssociationsCollection (Generic Exception)");
        *err = std::string("collection: ") + getAssociationsCollectionName(tenant).c_str() +
                " - query(): " + query.toString() +
                " - exception: " + "generic";
        return false;
    }

    /* Process query result */
    while (cursor->more()) {
        BSONObj r = cursor->next();
        LM_T(LmtMongo, ("retrieved document: '%s'", r.toString().c_str()));

        std::string name      = STR_FIELD(r, "_id");
        std::string srcEnId   = STR_FIELD(r.getField(ASSOC_SOURCE_ENT).embeddedObject(), ASSOC_ENT_ID);
        std::string srcEnType = STR_FIELD(r.getField(ASSOC_SOURCE_ENT).embeddedObject(), ASSOC_ENT_TYPE);
        std::string tgtEnId   = STR_FIELD(r.getField(ASSOC_TARGET_ENT).embeddedObject(), ASSOC_ENT_ID);
        std::string tgtEnType = STR_FIELD(r.getField(ASSOC_TARGET_ENT).embeddedObject(), ASSOC_ENT_TYPE);

        Metadata* md = new Metadata(name, "Association");
        md->association.entityAssociation.source.id        = srcEnId;
        md->association.entityAssociation.source.type      = srcEnType;
        md->association.entityAssociation.source.isPattern = "false";
        md->association.entityAssociation.target.id        = tgtEnId;
        md->association.entityAssociation.target.type      = tgtEnType;
        md->association.entityAssociation.target.isPattern = "false";

        std::vector<BSONElement> attrs = r.getField(ASSOC_ATTRS).Array();
        for (unsigned int ix = 0; ix < attrs.size(); ++ix) {
            std::string srcAttr = STR_FIELD(attrs[ix].embeddedObject(), ASSOC_ATTRS_SOURCE);
            std::string tgtAttr = STR_FIELD(attrs[ix].embeddedObject(), ASSOC_ATTRS_TARGET);
            AttributeAssociation* attrAssoc = new AttributeAssociation();
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
* associationsDiscoverConvextAvailability -
*/
static HttpStatusCode associationsDiscoverConvextAvailability(DiscoverContextAvailabilityRequest* requestP, DiscoverContextAvailabilityResponse* responseP, std::string scope, std::string tenant) {

    if (scope == SCOPE_VALUE_ASSOC_ALL) {
        LM_W(("%s scope not supported", SCOPE_VALUE_ASSOC_ALL));
        responseP->errorCode.fill(SccNotImplemented, std::string("Not supported scope: '") + SCOPE_VALUE_ASSOC_ALL + "'");
        return SccOk;
    }

    MetadataVector mdV;
    std::string err;
    if (!associationsQuery(requestP->entityIdVector, requestP->attributeList, scope, &mdV, &err, tenant)) {
        responseP->errorCode.fill(SccReceiverInternalError, std::string("Database error: ") + err);
        LM_RE(SccOk,(responseP->errorCode.details.c_str()));
    }

    /* Query for associated entities */
    for (unsigned int ix = 0; ix < mdV.size(); ++ix) {
        /* Each association involves a registrationsQuery() operation, accumulating the answer in
         * responseP->responseVector */
        Metadata* md = mdV.get(ix);
        EntityIdVector enV;
        AttributeList attrL;

        EntityId en;
        if (scope == SCOPE_VALUE_ASSOC_SOURCE) {
            en = EntityId(md->association.entityAssociation.source.id, md->association.entityAssociation.source.type);
        }
        else {  // SCOPE_VALUE_ASSOC_TARGET
            en = EntityId(md->association.entityAssociation.target.id, md->association.entityAssociation.target.type);
        }
        enV.push_back(&en);

        for (unsigned int jx = 0; jx < md->association.attributeAssociationList.size(); ++jx) {
            if (scope == SCOPE_VALUE_ASSOC_SOURCE) {
                attrL.push_back(md->association.attributeAssociationList.get(jx)->source);
            }
            else {
                attrL.push_back(md->association.attributeAssociationList.get(jx)->target);
            }
        }

        ContextRegistrationResponseVector crrV;
        if (!registrationsQuery(enV, attrL, &crrV, &err, tenant)) {
            responseP->errorCode.fill(SccReceiverInternalError, err);
            LM_RE(SccOk,(responseP->errorCode.details.c_str()));
        }

        /* Accumulate in responseP */
        for (unsigned int jx = 0; jx < crrV.size(); ++jx) {
            responseP->responseVector.push_back(crrV.get(jx));
        }
    }

    /* Set association metadata as final ContextRegistrationResponse*/
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
static HttpStatusCode conventionalDiscoverContextAvailability(DiscoverContextAvailabilityRequest* requestP, DiscoverContextAvailabilityResponse* responseP, std::string tenant) {
    std::string err;
    if (!registrationsQuery(requestP->entityIdVector, requestP->attributeList, &responseP->responseVector, &err, tenant)) {
        responseP->errorCode.fill(SccReceiverInternalError, err);
        LM_RE(SccOk,(responseP->errorCode.details.c_str()));
    }

    if (responseP->responseVector.size() == 0) {
        /* If the responseV is empty, we haven't found any entity and have to fill the status code part in the
         * response */
        responseP->errorCode.fill(SccContextElementNotFound);
        return SccOk;
    }

    return SccOk;
}

/* ****************************************************************************
*
* mongoDiscoverContextAvailability - 
*/
HttpStatusCode mongoDiscoverContextAvailability(DiscoverContextAvailabilityRequest* requestP, DiscoverContextAvailabilityResponse* responseP, std::string tenant)
{
  reqSemTake(__FUNCTION__, "mongo ngsi9 discovery request");

  LM_T(LmtMongo, ("DiscoverContextAvailability Request"));  

  /* Depending on the scope used, we invoke one function or other. DiscoverContextAvailability may behave
   * differently depending on the scope. Although OperationScope is a list in NGSI, we only support one
   * scope at the same time */
  int nScopes = requestP->restriction.scopeVector.size();
  if (nScopes > 0) {
    if (nScopes > 1) {
      LM_W(("Using %d scopes: only the first one will be used", nScopes));
    }
    std::string scopeType  = requestP->restriction.scopeVector.get(0)->type;
    std::string scopeValue = requestP->restriction.scopeVector.get(0)->value;

    if (scopeType == SCOPE_TYPE_ASSOC) {
      HttpStatusCode ms = associationsDiscoverConvextAvailability(requestP, responseP, scopeValue, tenant);
      reqSemGive(__FUNCTION__, "mongo ngsi9 discovery request (association)");
      return ms;
    }
    else {
      LM_W(("Unsupported scope (%s, %s), doing conventional discoverContextAvailability", scopeType.c_str(), scopeValue.c_str()));
    }
  }

  HttpStatusCode hsCode = conventionalDiscoverContextAvailability(requestP, responseP, tenant);
  if (hsCode != SccOk)
    ++noOfDiscoveryErrors;

  reqSemGive(__FUNCTION__, "mongo ngsi9 discovery request");
  return hsCode;
}
