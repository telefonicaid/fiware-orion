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

#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"

#include "common/tag.h"
#include "common/RenderFormat.h"
#include "alarmMgr/alarmMgr.h"
#include "ngsi/ContextElementResponse.h"
#include "ngsi/StringList.h"
#include "ngsi10/QueryContextResponse.h"

#include "mongoBackend/dbConstants.h"
#include "mongoBackend/dbFieldEncoding.h"
#include "mongoBackend/compoundResponses.h"
#include "mongoBackend/MongoGlobal.h"       // includedAttribute

#include "mongoDriver/safeMongo.h"



/* ****************************************************************************
*
* ContextElementResponse::ContextElementResponse -
*/
ContextElementResponse::ContextElementResponse()
{
  prune = false;
}



/* ****************************************************************************
*
* ContextElementResponse::ContextElementResponse - 
*/
ContextElementResponse::ContextElementResponse(EntityId* eP, ContextAttribute* aP)
{
  prune = false;

  entity.fill(eP->id, eP->type, eP->isPattern);

  if (aP != NULL)
  {
    entity.attributeVector.push_back(new ContextAttribute(aP));
  }
}



/* ****************************************************************************
*
* ContextElementResponse::ContextElementResponse - 
*/
ContextElementResponse::ContextElementResponse(ContextElementResponse* cerP, bool cloneCompounds)
{
  prune = false;

  entity.fill(cerP->entity, false, cloneCompounds);
  statusCode.fill(cerP->statusCode);
}



/* ****************************************************************************
*
* ContextElementResponse::ContextElementResponse -
*
* This constructor builds the CER object based in a BSON object taken from the
* entities collection at DB.
*
* Note that statusCode is not touched by this constructor.
*/
ContextElementResponse::ContextElementResponse
(
  const orion::BSONObj&  entityDoc,
  const StringList&      attrL
)
{
  prune = false;

  // Entity
  orion::BSONObj id = getFieldF(entityDoc, "_id").embeddedObject();

  std::string entityId   = getStringFieldF(id, ENT_ENTITY_ID);
  std::string entityType = id.hasField(ENT_ENTITY_TYPE) ? getStringFieldF(id, ENT_ENTITY_TYPE) : "";

  entity.fill(entityId, entityType, "false");
  entity.servicePath = id.hasField(ENT_SERVICE_PATH) ? getStringFieldF(id, ENT_SERVICE_PATH) : "";

  /* Get the location attribute (if it exists) */
  std::string locAttr;
  if (entityDoc.hasField(ENT_LOCATION))
  {
    locAttr = getStringFieldF(getObjectFieldF(entityDoc, ENT_LOCATION), ENT_LOCATION_ATTRNAME);
  }


  //
  // Attribute vector
  //
  entity.attributeVector.fill(getObjectFieldF(entityDoc, ENT_ATTRS), attrL);

  /* Set creDate and modDate at entity level */
  if (entityDoc.hasField(ENT_CREATION_DATE))
  {
    entity.creDate = getNumberFieldF(entityDoc, ENT_CREATION_DATE);
  }

  if (entityDoc.hasField(ENT_MODIFICATION_DATE))
  {
    entity.modDate = getNumberFieldF(entityDoc, ENT_MODIFICATION_DATE);
  }
}


/* ****************************************************************************
*
* ContextElementResponse::ContextElementResponse -
*
* This constructor builds the CER from a CEP. Note that statusCode is not touched.
*/
ContextElementResponse::ContextElementResponse(Entity* eP, bool useDefaultType)
{
  entity.fill(*eP, useDefaultType);
}



/* ****************************************************************************
*
* ContextElementResponse::toJsonV1 -
*/
std::string ContextElementResponse::toJsonV1
(
  bool                             asJsonObject,
  RequestType                      requestType,
  const std::vector<std::string>&  attrsFilter,
  bool                             blacklist,
  const std::vector<std::string>&  metadataFilter,
  bool                             comma,
  bool                             omitAttributeValues
)
{
  std::string out = "";

  out += startTag();
  out += entity.toJsonV1(asJsonObject, requestType, attrsFilter, blacklist, metadataFilter, true, omitAttributeValues);
  out += statusCode.toJsonV1(false);
  out += endTag(comma, false);

  return out;
}



/* ****************************************************************************
*
* ContextElementResponse::toJsonV1 -
*
* Wrapper of toJsonV1 with empty attrsFilter and metadataFilter
*/
std::string ContextElementResponse::toJsonV1
(
  bool         asJsonObject,
  RequestType  requestType,
  bool         blacklist,
  bool         comma,
  bool         omitAttributeValues
)
{
  std::string out = "";

  out += startTag();
  out += entity.toJsonV1(asJsonObject, requestType, blacklist, true, omitAttributeValues);
  out += statusCode.toJsonV1(false);
  out += endTag(comma, false);

  return out;
}



/* ****************************************************************************
*
* ContextElementResponse::toJson - 
*/
std::string ContextElementResponse::toJson
(
  RenderFormat                         renderFormat,
  const std::vector<std::string>&      attrsFilter,
  bool                                 blacklist,
  const std::vector<std::string>&      metadataFilter,
  ExprContextObject*                   exprContextObjectP
)
{
  std::string out;

  out = entity.toJson(renderFormat, attrsFilter, blacklist, metadataFilter, false, exprContextObjectP);

  return out;
}



/* ****************************************************************************
*
* ContextElementResponse::applyUpdateOperators -
*/
void ContextElementResponse::applyUpdateOperators(void)
{
  entity.applyUpdateOperators();
}



/* ****************************************************************************
*
* ContextElementResponse::release - 
*/
void ContextElementResponse::release(void)
{
  entity.release();
  statusCode.release();
}



/* ****************************************************************************
*
* ContextElementResponse::check - 
*/
std::string ContextElementResponse::check
(
  ApiVersion          apiVersion,
  RequestType         requestType,
  const std::string&  predetectedError,
  int                 counter
)
{
  std::string res;

  if ((res = entity.check(apiVersion, requestType)) != "OK")
  {
    return res;
  }

  if ((res = statusCode.check()) != "OK")
  {
    return res;
  }

  return "OK";
}



/* ****************************************************************************
*
* ContextElementResponse::fill - 
*/
void ContextElementResponse::fill(QueryContextResponse* qcrP, const std::string& entityId, const std::string& entityType)
{
  if (qcrP == NULL)
  {
    statusCode.fill(SccContextElementNotFound);
    return;
  }

  if (qcrP->contextElementResponseVector.size() == 0)
  {
    statusCode.fill(&qcrP->errorCode);
    entity.fill(entityId, entityType, "false");

    if ((statusCode.code != SccOk) && (statusCode.details.empty()))
    {
      statusCode.details = "Entity id: /" + entityId + "/";
    }

    return;
  }

  //
  // FIXME P7: If more than one context element is found, we simply select the first one.
  //           A better approach would be to change this convop to return a vector of responses.
  //           Adding a call to alarmMgr::badInput - with this I mean that the user that sends the 
  //           query needs to avoid using this conv op to make any queries that can give more than
  //           one unique context element :-).
  //           This FIXME is related to github issue #588 and (probably) #650.
  //           Also, optimizing this would be part of issue #768
  //
  if (qcrP->contextElementResponseVector.size() > 1)
  {
    alarmMgr.badInput(clientIp, "more than one context element found the this query - selecting the first one");
  }

  entity.fill(qcrP->contextElementResponseVector[0]->entity);

  if (qcrP->errorCode.code != SccNone)
  {
    statusCode.fill(&qcrP->errorCode);
  }
}



/* ****************************************************************************
*
* ContextElementResponse::fill - 
*/
void ContextElementResponse::fill(ContextElementResponse* cerP)
{
  entity.fill(cerP->entity);
  statusCode.fill(cerP->statusCode);
}



/* ****************************************************************************
*
* ContextElementResponse::clone - 
*/
ContextElementResponse* ContextElementResponse::clone(void)
{
  ContextElementResponse* cerP = new ContextElementResponse();

  cerP->fill(this);

  return cerP;
}
