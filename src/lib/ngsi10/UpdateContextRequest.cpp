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
#include <stdio.h>
#include <string>

#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"

#include "common/globals.h"
#include "common/JsonHelper.h"
#include "alarmMgr/alarmMgr.h"
#include "ngsi/ContextAttribute.h"
#include "ngsi10/UpdateContextRequest.h"
#include "ngsi10/UpdateContextResponse.h"



/* ****************************************************************************
*
* UpdateContextRequest::UpdateContextRequest -
*/
UpdateContextRequest::UpdateContextRequest()
{
}



/* ****************************************************************************
*
* UpdateContextRequest::UpdateContextRequest -
*/
UpdateContextRequest::UpdateContextRequest(const std::string& _contextProvider, bool _legacyProviderFormat, Entity* eP)
{
  contextProvider      = _contextProvider;
  legacyProviderFormat = _legacyProviderFormat;

  Entity* neweP = new Entity(eP->entityId.id, eP->entityId.idPattern, eP->entityId.type, eP->entityId.typePattern);
  neweP->renderId = eP->renderId;
  entityVector.push_back(neweP);
}



/* ****************************************************************************
*
* UpdateContextRequest::toJson -
*/
std::string UpdateContextRequest::toJson(void)
{
  JsonObjectHelper jh;

  jh.addRaw("entities", entityVector.toJson(NGSI_V2_NORMALIZED));

  jh.addString("actionType", actionTypeString(updateActionType));

  return jh.str();
}



/* ****************************************************************************
*
* UpdateContextRequest::toJsonV1 -
*
* This is used only in the legacyForwarding:true logic. It would remove once that deprecated feature
* would be removed
*
* Example:
*
* {
*   "contextElements": [
*     {
*       "type": "Room",
*       "isPattern": "false",
*       "id": "ConferenceRoom",
*       "attributes": [
*       {
*         "name": "temperature",
*         "type": "degree",
*         "value": "c23",
*         "metadatas": [
*           {
*             "name": "ID",
*             "type": "integer",
*             "value": "3"
*           }
*         ]
*       }
*      ]
*    }
*  ],
*  "updateAction": "APPEND"
* }
*/
std::string UpdateContextRequest::toJsonV1(void)
{
  JsonObjectHelper jh;

  JsonVectorHelper jhContextElements;
  for (unsigned int ix = 0; ix < entityVector.size(); ++ix)
  {
    JsonObjectHelper jhEntity;
    Entity* eP = entityVector[ix];

    if (eP->entityId.idPattern.empty())
    {
      jhEntity.addString("id", eP->entityId.id);
      jhEntity.addString("isPattern", "false");
    }
    else
    {
      jhEntity.addString("id", eP->entityId.idPattern);
      jhEntity.addString("isPattern", "true");
    }
    jhEntity.addString("type", eP->entityId.type);

    JsonVectorHelper jhAttributes;
    for (unsigned int jx = 0; jx < eP->attributeVector.size(); ++jx)
    {
      JsonObjectHelper jhAttribute;
      ContextAttribute* caP = eP->attributeVector[jx];

      jhAttribute.addString("name", caP->name);
      jhAttribute.addString("type", caP->type);
      jhAttribute.addRaw("value", caP->toJsonValue());

      if (caP->metadataVector.size() > 0)
      {
        JsonVectorHelper jhMetadatas;
        for (unsigned int kx = 0; kx < caP->metadataVector.size(); ++kx)
        {
          JsonObjectHelper jhMetadata;
          Metadata *mdP = caP->metadataVector[kx];

          jhMetadata.addString("name", mdP->name);
          jhMetadata.addString("type", mdP->type);
          jhMetadata.addRaw("value", mdP->toJson());

          jhMetadatas.addRaw(jhMetadata.str());
        }
        jhAttribute.addRaw("metadatas", jhMetadatas.str());
      }

      jhAttributes.addRaw(jhAttribute.str());
    }
    jhEntity.addRaw("attributes", jhAttributes.str());

    jhContextElements.addRaw(jhEntity.str());
  }
  jh.addRaw("contextElements", jhContextElements.str());

  switch (updateActionType)
  {
  case ActionTypeUpdate:
    jh.addString("updateAction", "UPDATE");
    break;
  case ActionTypeAppend:
    jh.addString("updateAction", "APPEND");
    break;
  case ActionTypeAppendStrict:
    jh.addString("updateAction", "APPEND_STRICT");
    break;
  case ActionTypeDelete:
    jh.addString("updateAction", "DELETE");
    break;
  case ActionTypeReplace:
    jh.addString("updateAction", "REPLACE");
    break;
  default:
    jh.addString("updateAction", "UNKNOWN");
  }

  return jh.str();
}



/* ****************************************************************************
*
* UpdateContextRequest::release -
*/
void UpdateContextRequest::release(void)
{
  entityVector.release();
}



/* ****************************************************************************
*
* UpdateContextRequest::fill -
*/
void UpdateContextRequest::fill
(
  const std::string& entityId,
  const std::string& entityIdPattern,
  const std::string& entityType,
  const std::string& attributeName,
  ActionType         _updateActionType
)
{
  Entity* eP = new Entity();

  EntityId enId(entityId, entityIdPattern, entityType, "");

  eP->fill(enId);
  entityVector.push_back(eP);

  updateActionType = _updateActionType;

  if (!attributeName.empty())
  {
    ContextAttribute* caP = new ContextAttribute(attributeName, "", "");
    eP->attributeVector.push_back(caP);
  }
}



/* ****************************************************************************
*
* UpdateContextRequest::fill -
*/
void UpdateContextRequest::fill(const Entity* entP, ActionType _updateActionType)
{
  Entity*  eP = new Entity(entP->entityId.id, "", entP->entityId.type, "");

  eP->attributeVector.fill(entP->attributeVector);

  entityVector.push_back(eP);
  updateActionType = _updateActionType;
}



/* ****************************************************************************
*
* UpdateContextRequest::fill -
*/
void UpdateContextRequest::fill
(
  const std::string&   entityId,
  ContextAttribute*    attributeP,
  ActionType           _updateActionType,
  const std::string&   type
)
{
  Entity*           eP = new Entity(entityId, "", type, "");
  ContextAttribute* aP = new ContextAttribute(attributeP);

  eP->attributeVector.push_back(aP);
  entityVector.push_back(eP);
  updateActionType = _updateActionType;
}



/* ****************************************************************************
*
* UpdateContextRequest::fill -
*
* Instead of copying the attributes, the created ContextElements will just point to
* the already existing ContextAttributes and the original vector is then cleared to
* avoid any double-free problems.
*/
void UpdateContextRequest::fill
(
  EntityVector*  entities,
  ActionType     _updateActionType
)
{
  updateActionType = _updateActionType;

  for (unsigned int eIx = 0; eIx < entities->vec.size(); ++eIx)
  {
    Entity*  eP    = entities->vec[eIx];
    Entity*  neweP = new Entity(eP->entityId.id, eP->entityId.idPattern, eP->entityId.type, eP->entityId.typePattern);

    for (unsigned int aIx = 0; aIx < eP->attributeVector.size(); ++aIx)
    {
      // NOT copying the attribute, just pointing to it - original vector is then cleared
      neweP->attributeVector.push_back(eP->attributeVector[aIx]);
    }

    eP->attributeVector.vec.clear();  // original vector is cleared
    entityVector.push_back(neweP);
  }
}



/* ****************************************************************************
*
* UpdateContextRequest::attributeLookup -
*/
ContextAttribute* UpdateContextRequest::attributeLookup(Entity* eP, const std::string& attributeName)
{
  for (unsigned int ceIx = 0; ceIx < entityVector.size(); ++ceIx)
  {
    Entity* enP = entityVector[ceIx];

    // empty type in request (enP) is always a match
    if ((enP->entityId.id != eP->entityId.id) || ((enP->entityId.type != "") && (enP->entityId.type != eP->entityId.type)))
    {
      continue;
    }

    Entity* eVItemP = entityVector[ceIx];

    for (unsigned int aIx = 0; aIx < eVItemP->attributeVector.size(); ++aIx)
    {
      ContextAttribute* aP = eVItemP->attributeVector[aIx];

      if (aP->name == attributeName)
      {
        return aP;
      }
    }
  }

  return NULL;
}
