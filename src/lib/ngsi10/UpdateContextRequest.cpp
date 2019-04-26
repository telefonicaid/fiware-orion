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
#include "common/tag.h"
#include "alarmMgr/alarmMgr.h"
#include "convenience/UpdateContextElementRequest.h"
#include "convenience/AppendContextElementRequest.h"
#include "ngsi/ContextAttribute.h"
#include "ngsi10/UpdateContextRequest.h"
#include "ngsi10/UpdateContextResponse.h"
#include "convenience/UpdateContextAttributeRequest.h"



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
UpdateContextRequest::UpdateContextRequest(const std::string& _contextProvider, ProviderFormat _providerFormat, Entity* eP)
{
  contextProvider = _contextProvider;
  providerFormat  = _providerFormat;
  entityVector.push_back(new Entity(eP->id, eP->type, eP->isPattern));
}



/* ****************************************************************************
*
* UpdateContextRequest::toJsonV1 -
*/
std::string UpdateContextRequest::toJsonV1(bool asJsonObject)
{
  std::string  out = "";

  //
  // About JSON commas:
  //   Both fields are MANDATORY, so, always comma after "entityVector"
  //
  out += startTag();
  out += entityVector.toJsonV1(asJsonObject, UpdateContext, true);
  out += valueTag("updateAction", actionTypeString(V1, updateActionType), false);
  out += endTag(false);

  return out;
}



/* ****************************************************************************
*
* UpdateContextRequest::check -
*/
std::string UpdateContextRequest::check(ApiVersion apiVersion, bool asJsonObject, const std::string& predetectedError)
{
  std::string            res;
  UpdateContextResponse  response;

  if (predetectedError != "")
  {
    response.errorCode.fill(SccBadRequest, predetectedError);
    return response.toJsonV1(asJsonObject);
  }

  if ((res = entityVector.check(apiVersion, UpdateContext)) != "OK")
  {
    response.errorCode.fill(SccBadRequest, res);
    return response.toJsonV1(asJsonObject);
  }

  return "OK";
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
  const UpdateContextElementRequest* ucerP,
  const std::string&                 entityId,
  const std::string&                 entityType
)
{
  Entity* eP = new Entity(entityId, entityType, "false");

  eP->attributeVector.fill(ucerP->contextAttributeVector);

  entityVector.push_back(eP);

  updateActionType = ActionTypeUpdate;  // Coming from an UpdateContextElementRequest (PUT), must be UPDATE
}



/* ****************************************************************************
*
* UpdateContextRequest::fill -
*/
void UpdateContextRequest::fill
(
  const AppendContextElementRequest*  acerP,
  const std::string&                  entityId,
  const std::string&                  entityType
)
{
  Entity* eP = new Entity(entityId, entityType, "false");

  eP->attributeVector.fill(acerP->contextAttributeVector);

  entityVector.push_back(eP);
  updateActionType = ActionTypeAppend;  // Coming from an AppendContextElementRequest (POST), must be APPEND
}



/* ****************************************************************************
*
* UpdateContextRequest::fill -
*/
void UpdateContextRequest::fill
(
  const std::string& entityId,
  const std::string& entityType,
  const std::string& isPattern,
  const std::string& attributeName,
  ActionType         _updateActionType
)
{
  Entity* eP = new Entity();

  eP->fill(entityId, entityType, isPattern);
  entityVector.push_back(eP);

  updateActionType = _updateActionType;

  if (attributeName != "")
  {
    ContextAttribute* caP = new ContextAttribute(attributeName, "", "");
    eP->attributeVector.push_back(caP);
  }
}



/* ****************************************************************************
*
* UpdateContextRequest::fill -
*/
void UpdateContextRequest::fill
(
  const UpdateContextAttributeRequest* ucarP,
  const std::string&                   entityId,
  const std::string&                   entityType,
  const std::string&                   attributeName,  
  ActionType                           _updateActionType
)
{
  Entity*           eP = new Entity(entityId, entityType, "false");
  ContextAttribute* caP;

  if (ucarP->compoundValueP != NULL)
  {
    caP = new ContextAttribute(attributeName, ucarP->type, ucarP->compoundValueP);
  }
  else
  {
    caP = new ContextAttribute(attributeName, ucarP->type, ucarP->contextValue);
    caP->valueType = ucarP->valueType;
  }

  caP->metadataVector.fill((MetadataVector*) &ucarP->metadataVector);
  eP->attributeVector.push_back(caP);

  entityVector.push_back(eP);

  updateActionType = _updateActionType;
}



/* ****************************************************************************
*
* UpdateContextRequest::fill -
*/
void UpdateContextRequest::fill(const Entity* entP, ActionType _updateActionType)
{
  Entity*  eP = new Entity(entP->id, entP->type, "false");

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
  Entity*           eP = new Entity(entityId, type, "false");
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
  Entities*    entities,
  ActionType  _updateActionType
)
{
  updateActionType = _updateActionType;

  for (unsigned int eIx = 0; eIx < entities->vec.size(); ++eIx)
  {
    Entity*  eP    = entities->vec[eIx];
    Entity*  neweP = new Entity(eP->id, eP->type, eP->isPattern);

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

    if ((enP->id != eP->id) || (enP->type != eP->type))
    {
      continue;
    }

    Entity* eP = entityVector[ceIx];

    for (unsigned int aIx = 0; aIx < eP->attributeVector.size(); ++aIx)
    {
      ContextAttribute* aP = eP->attributeVector[aIx];

      if (aP->name == attributeName)
      {
        return aP;
      }
    }
  }

  return NULL;
}
