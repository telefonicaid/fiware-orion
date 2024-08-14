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
