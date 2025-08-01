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
#include "common/globals.h"
#include "common/JsonHelper.h"
#include "alarmMgr/alarmMgr.h"
#include "ngsi/Request.h"
#include "ngsi/StringList.h"
#include "ngsi/EntityIdVector.h"
#include "ngsi/QueryContextResponse.h"
#include "ngsi/QueryContextRequest.h"
#include "apiTypesV2/BatchQuery.h"



/* ****************************************************************************
*
* QueryContextRequest::QueryContextRequest
*
*/
QueryContextRequest::QueryContextRequest()
{
}



/* ****************************************************************************
*
* QueryContextRequest::QueryContextRequest
*/
QueryContextRequest::QueryContextRequest(const std::string& _contextProvider, EntityId* eP, const std::string& attributeName, bool _legacyProviderFormat)
{
  contextProvider       = _contextProvider;
  legacyProviderFormat  = _legacyProviderFormat;

  entityIdVector.push_back(new EntityId(eP));

  if (!attributeName.empty())
  {
    attributeList.push_back(attributeName);
  }
}



/* ****************************************************************************
*
* QueryContextRequest::QueryContextRequest
*/
QueryContextRequest::QueryContextRequest(const std::string& _contextProvider, EntityId* eP, const StringList& _attributeList, bool _legacyProviderFormat)
{
  contextProvider       = _contextProvider;
  legacyProviderFormat  = _legacyProviderFormat;

  entityIdVector.push_back(new EntityId(eP));

  attributeList.clone(_attributeList);
}



/* ****************************************************************************
*
* QueryContextRequest::toJson -
*/
std::string QueryContextRequest::toJson(void)
{
  JsonObjectHelper jh;

  jh.addRaw("entities", entityIdVector.toJson());
  jh.addRaw("attrs", attributeList.toJson());

  return jh.str();
}



/* ****************************************************************************
*
* QueryContextRequest::toJsonV1 -
*
* This is used only in the legacyForwarding:true logic. It would remove once that deprecated feature
* would be removed.
*
* Example:
* {
*   "entities": [
*     {
*       "type": "Room",
*       "isPattern": "true",
*       "id": "ConferenceRoom.*"
*     }
*   ],
*   "attributes": [
*     "temperature",
*     "pressure",
*     "lightstatus"
*   ]
}
*/
std::string QueryContextRequest::toJsonV1(void)
{
  // Diferent from original toJsonV1() we don't render restriction field (as it is not needed in the forwarding functionality)

  JsonObjectHelper jh;

  JsonVectorHelper jhEntities;
  for (unsigned int ix = 0; ix < entityIdVector.size(); ++ix)
  {
    JsonObjectHelper jhEntity;
    if (entityIdVector[ix]->idPattern.empty())
    {
      jhEntity.addString("id", entityIdVector[ix]->id);
      jhEntity.addString("isPattern", "false");
    }
    else
    {
      jhEntity.addString("id", entityIdVector[ix]->idPattern);
      jhEntity.addString("isPattern", "true");
    }
    jhEntity.addString("type", entityIdVector[ix]->type);

    jhEntities.addRaw(jhEntity.str());
  }
  jh.addRaw("entities", jhEntities.str());

  JsonVectorHelper jhAttributes;
  for (unsigned int ix = 0; ix < attributeList.size(); ++ix)
  {
    jhAttributes.addString(attributeList[ix]);
  }
  jh.addRaw("attributes", jhAttributes.str());

  return jh.str();
}



/* ****************************************************************************
*
* QueryContextRequest::release -
*/
void QueryContextRequest::release(void)
{
  entityIdVector.release();
}



/* ****************************************************************************
*
* QueryContextRequest::fill -
*/
void QueryContextRequest::fill
(
  const std::string& entityId,
  const std::string& entityIdPattern,
  const std::string& entityType
)
{
  EntityId* eidP = new EntityId(entityId, entityIdPattern, entityType, "");

  entityIdVector.push_back(eidP);
}



/* ****************************************************************************
*
* QueryContextRequest::fill -
*
* NOTE
* If the incoming bqP->entities.vec is empty, then one almighty entity::id is
* added to the QueryContextRequest::entityIdVector, namely, idPattern .* with empty type,
* matching ALL entities.
*
*/
void QueryContextRequest::fill(BatchQuery* bqP)
{
  if (bqP->entities.vec.size() != 0)
  {
    entityIdVector.fill(bqP->entities);
  }
  else
  {
    EntityId* eP = new EntityId("", ".*", "", "");
    entityIdVector.push_back(eP);
  }

  attributeList.fill(bqP->attributeV.stringV);  // attributeV is deprecated
  attrsList.fill(bqP->attrsV.stringV);
  metadataList.fill(bqP->metadataV.stringV);
}
