/*
*
* Copyright 2015 Telefonica Investigacion y Desarrollo, S.A.U
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
#include <vector>
#include <map>

#include "logMsg/traceLevels.h"
#include "logMsg/logMsg.h"
#include "common/string.h"
#include "common/globals.h"
#include "common/JsonHelper.h"
#include "common/errorMessages.h"
#include "rest/uriParamNames.h"
#include "alarmMgr/alarmMgr.h"
#include "parse/forbiddenChars.h"
#include "ngsi/QueryContextResponse.h"
#include "mongoBackend/dbFieldEncoding.h"
#include "rest/OrionError.h"

#include "apiTypesV2/Entity.h"



/* ****************************************************************************
*
* Entity::Entity - 
*/
Entity::Entity(): renderId(true), creDate(0), modDate(0)
{
}



/* ****************************************************************************
*
* Entity::Entity -
*
* This constructor was ported from old ContextElement class
*/
Entity::Entity(const std::string& _id, const std::string& _idPattern, const std::string& _type, const std::string& _typePattern)
{
  entityId.id            = _id;
  entityId.idPattern     = _idPattern;
  entityId.type          = _type;
  entityId.typePattern   = _typePattern;
  renderId               = true;
  creDate                = 0;
  modDate                = 0;
}



/* ****************************************************************************
*
* Entity::~Entity - 
*/
Entity::~Entity()
{
  release();
}


/* ****************************************************************************
*
* Entity::addAllAttrsExceptFiltered -
*
*/
void Entity::addAllAttrsExceptFiltered(std::vector<ContextAttribute*>*  orderedAttrs)
{
  for (unsigned int ix = 0; ix < attributeVector.size(); ix++)
  {
    if ((!attributeVector[ix]->shadowed) && (!attributeVector[ix]->skip))
    {
      orderedAttrs->push_back(attributeVector[ix]);
    }
  }
}


/* ****************************************************************************
*
* Entity::filterAndOrderAttrs -
*
*/
void Entity::filterAndOrderAttrs
(
  const std::vector<std::string>&  attrsFilter,
  bool                             blacklist,
  std::vector<ContextAttribute*>*  orderedAttrs
)
{
  if (blacklist)
  {
    if (attrsFilter.size() == 0)
    {
      // No filter, no blacklist. Attributes are "as is" in the entity except shadowed ones,
      // which require explicit inclusion (dateCreated, etc.) and skipped
      addAllAttrsExceptFiltered(orderedAttrs);
    }
    else
    {
      // Filter, blacklist. The order is the one in the entity, after removing attributes.
      // In blacklist case shadowed attributes (dateCreated, etc) and skipped are never included
      for (unsigned int ix = 0; ix < attributeVector.size(); ix++)
      {
        std::string name = attributeVector[ix]->name;
        if ((!attributeVector[ix]->shadowed) && (!attributeVector[ix]->skip) && (std::find(attrsFilter.begin(), attrsFilter.end(), name) == attrsFilter.end()))
        {
          orderedAttrs->push_back(attributeVector[ix]);
        }
      }
    }
  }
  else
  {
    if (attrsFilter.size() == 0)
    {
      // No filter, no blacklist. Attributes are "as is" in the entity
      // except shadowed ones (dateCreated, etc.) and skipped
      addAllAttrsExceptFiltered(orderedAttrs);
    }
    else
    {
      // Filter, no blacklist. Processing will depend on whether '*' is in the attrsFilter or not
      if (std::find(attrsFilter.begin(), attrsFilter.end(), ALL_ATTRS) != attrsFilter.end())
      {
        // - If '*' is in: all attributes are included in the same order used by the entity
        for (unsigned int ix = 0; ix < attributeVector.size(); ix++)
        {
          if (attributeVector[ix]->skip)
          {
            // Skipped attributes are never included
            continue;
          }
          else if (attributeVector[ix]->shadowed)
          {
            // Shadowed attributes needs explicit inclusion
            if ((std::find(attrsFilter.begin(), attrsFilter.end(), attributeVector[ix]->name) != attrsFilter.end()))
            {
              orderedAttrs->push_back(attributeVector[ix]);
            }
          }
          else
          {
            orderedAttrs->push_back(attributeVector[ix]);
          }
        }
      }
      else
      {
        // - If '*' is not in: attributes are include in the attrsFilter order
        // (except skiped)
        for (unsigned int ix = 0; ix < attrsFilter.size(); ix++)
        {
          int found;
          if ((found = attributeVector.get(attrsFilter[ix])) != -1)
          {
            if (attributeVector[found]->skip)
            {
              // Skipped attributes are never included
              continue;
            }
            orderedAttrs->push_back(attributeVector[found]);
          }
        }
      }
    }
  }
}



/* ****************************************************************************
*
* Entity::toJson -
*
* The rendering of JSON in APIv2 depends on the URI param 'options'
* Rendering methods:
*   o 'normalized' (default)
*   o 'keyValues'  (less verbose, only name and values shown for attributes - no type, no metadatas)
*   o 'values'     (only the values of the attributes are printed, in a vector)
*
* renderNgsiField true is used in custom notification payloads, which have some small differences
* with regards to conventional rendering
*/
std::string Entity::toJson
(
  RenderFormat                         renderFormat,
  const std::vector<std::string>&      attrsFilter,
  bool                                 blacklist,
  const std::vector<std::string>&      metadataFilter,
  bool                                 renderNgsiField
)
{
  std::vector<ContextAttribute* > orderedAttrs;
  filterAndOrderAttrs(attrsFilter, blacklist, &orderedAttrs);

  std::string out;
  switch (renderFormat)
  {
  case NGSI_V2_VALUES:
    out = toJsonValues(orderedAttrs);
    break;
  case NGSI_V2_UNIQUE_VALUES:
    // unique is not allowed in attrsFormat, so no need of passing exprContextObjectP here
    out = toJsonUniqueValues(orderedAttrs);
    break;
  case NGSI_V2_KEYVALUES:
    out = toJsonKeyvalues(orderedAttrs);
    break;
  default:  // NGSI_V2_NORMALIZED
    out = toJsonNormalized(orderedAttrs, metadataFilter, renderNgsiField);
    break;
  }

  return out;
}



/* ****************************************************************************
*
* Entity::toJson -
*
* Simplified version of toJson without filters
*
* renderNgsiField true is used in custom notification payloads, which have some small differences
* with regards to conventional rendering
*/
std::string Entity::toJson(RenderFormat renderFormat, bool renderNgsiField)
{
  std::vector<std::string>  nullFilter;
  return toJson(renderFormat, nullFilter, false, nullFilter, renderNgsiField);
}



/* ****************************************************************************
*
* Entity::toJsonValues -
*/
std::string Entity::toJsonValues(const std::vector<ContextAttribute*>& orderedAttrs)
{
  JsonVectorHelper jh;

  for (unsigned int ix = 0; ix < orderedAttrs.size(); ix++)
  {
    ContextAttribute* caP = orderedAttrs[ix];
    jh.addRaw(caP->toJsonValue());
  }

  return jh.str();
}



/* ****************************************************************************
*
* Entity::toJsonUniqueValues -
*/
std::string Entity::toJsonUniqueValues(const std::vector<ContextAttribute*>& orderedAttrs)
{
  JsonVectorHelper jh;

  std::map<std::string, bool>  uniqueMap;

  for (unsigned int ix = 0; ix < orderedAttrs.size(); ix++)
  {
    ContextAttribute* caP = orderedAttrs[ix];

    std::string value = caP->toJsonValue();

    if (uniqueMap[value] == true)
    {
      // Already rendered. Skip.
      continue;
    }
    else
    {
      jh.addRaw(value);
      uniqueMap[value] = true;
    }
  }

  return jh.str();
}



/* ****************************************************************************
*
* Entity::toJsonKeyvalues -
*/
std::string Entity::toJsonKeyvalues(const std::vector<ContextAttribute*>& orderedAttrs)
{
  JsonObjectHelper jh;

  if (renderId)
  {
    jh.addString("id", entityId.id);

    /* This is needed for entities coming from NGSIv1 (which allows empty or missing types) */
    jh.addString("type", (!entityId.type.empty())? entityId.type : DEFAULT_ENTITY_TYPE);
  }

  for (unsigned int ix = 0; ix < orderedAttrs.size(); ix++)
  {
    ContextAttribute* caP = orderedAttrs[ix];
    jh.addRaw(caP->name, caP->toJsonValue());
  }

  return jh.str();
}



/* ****************************************************************************
*
* Entity::toJsonNormalized -
*
* renderNgsiField true is used in custom notification payloads, which have some small differences
* with regards to conventional rendering
*/
std::string Entity::toJsonNormalized
(
  const std::vector<ContextAttribute*>&  orderedAttrs,
  const std::vector<std::string>&        metadataFilter,
  bool                                   renderNgsiField
)
{
  JsonObjectHelper jh;

  if (renderId)
  {
    if (renderNgsiField)
    {
      /* In ngsi field in notifications "" is allowed for id and type, in which case we don't
       * print the field */
      if (!entityId.id.empty())
      {
        jh.addString("id", entityId.id);
      }
      if (!entityId.type.empty())
      {
        jh.addString("type", entityId.type);
      }
    }
    else
    {
      jh.addString("id", entityId.id);

      /* This is needed for entities coming from NGSIv1 (which allows empty or missing types) */
      jh.addString("type", (!entityId.type.empty())? entityId.type : DEFAULT_ENTITY_TYPE);
    }
  }

  for (unsigned int ix = 0; ix < orderedAttrs.size(); ix++)
  {
    ContextAttribute* caP = orderedAttrs[ix];
    jh.addRaw(caP->name, caP->toJson(metadataFilter, renderNgsiField));
  }

  return jh.str();
}



/* ****************************************************************************
*
* toString -
*
*/
std::string Entity::toString(void)
{
  std::string s;

  std::string effectiveId;
  std::string effectiveType;

  if (entityId.idPattern.empty())
  {
    effectiveId = entityId.id;
  }
  else
  {
    effectiveId = entityId.idPattern;
  }

  if (entityId.typePattern.empty())
  {
    effectiveType = entityId.type;
  }
  else
  {
    effectiveType = entityId.typePattern;
  }

  return effectiveId + ", " + effectiveType;

  return s;
}



/* ****************************************************************************
*
* ContextElement::checkId
*
*/
std::string Entity::checkId(RequestType requestType)
{
  ssize_t len;
  char errorMsg[128];

  if (((len = strlen(entityId.id.c_str())) < MIN_ID_LEN) && (requestType != EntityRequest))
  {
    snprintf(errorMsg, sizeof errorMsg, "entity id length: %zd, min length supported: %d", len, MIN_ID_LEN);
    alarmMgr.badInput(clientIp, errorMsg);
    return std::string(errorMsg);
  }

  if ((len = strlen(entityId.id.c_str())) > MAX_ID_LEN)
  {
    snprintf(errorMsg, sizeof errorMsg, "entity id length: %zd, max length supported: %d", len, MAX_ID_LEN);
    alarmMgr.badInput(clientIp, errorMsg);
    return std::string(errorMsg);
  }

  // Check for forbidden chars for "id", but not for "idPattern" is a pattern
  if (forbiddenIdCharsV2(entityId.id.c_str()))
  {
    alarmMgr.badInput(clientIp, ERROR_DESC_BAD_REQUEST_INVALID_CHAR_ENTID, entityId.id);
    return ERROR_DESC_BAD_REQUEST_INVALID_CHAR_ENTID;
  }

  return "OK";
}



/* ****************************************************************************
*
* ContextElement::checkIdPattern
*
*/
std::string Entity::checkIdPattern(RequestType requestType)
{
  ssize_t len;
  char errorMsg[128];

  if (((len = strlen(entityId.idPattern.c_str())) < MIN_ID_LEN) && (requestType != EntityRequest))
  {
    snprintf(errorMsg, sizeof errorMsg, "entity idPattern length: %zd, min length supported: %d", len, MIN_ID_LEN);
    alarmMgr.badInput(clientIp, errorMsg);
    return std::string(errorMsg);
  }

  if ((len = strlen(entityId.idPattern.c_str())) > MAX_ID_LEN)
  {
    snprintf(errorMsg, sizeof errorMsg, "entity idPattern length: %zd, max length supported: %d", len, MAX_ID_LEN);
    alarmMgr.badInput(clientIp, errorMsg);
    return std::string(errorMsg);
  }

  return "OK";
}



/* ****************************************************************************
*
* ContextElement::checkType
*
*/
std::string Entity::checkType(RequestType requestType)
{
  ssize_t len;
  char errorMsg[128];

  if ((len = strlen(entityId.type.c_str())) > MAX_ID_LEN)
  {
    snprintf(errorMsg, sizeof errorMsg, "entity type length: %zd, max length supported: %d", len, MAX_ID_LEN);
    alarmMgr.badInput(clientIp, errorMsg);
    return std::string(errorMsg);
  }

  if ((len = strlen(entityId.type.c_str())) < MIN_ID_LEN)
  {
    snprintf(errorMsg, sizeof errorMsg, "entity type length: %zd, min length supported: %d", len, MIN_ID_LEN);
    alarmMgr.badInput(clientIp, errorMsg);
    return std::string(errorMsg);
  }

  // Check for forbidden chars for "type", but not for "typePattern"
  if (forbiddenIdCharsV2(entityId.type.c_str()))
  {
    alarmMgr.badInput(clientIp, ERROR_DESC_BAD_REQUEST_INVALID_CHAR_ENTTYPE, entityId.type);
    return ERROR_DESC_BAD_REQUEST_INVALID_CHAR_ENTTYPE;
  }

  return "OK";
}



/* ****************************************************************************
*
* ContextElement::checkTypePattern
*
*/
std::string Entity::checkTypePattern(RequestType requestType)
{
  ssize_t len;
  char errorMsg[128];

  if ((len = strlen(entityId.typePattern.c_str())) > MAX_ID_LEN)
  {
    snprintf(errorMsg, sizeof errorMsg, "entity typePattern length: %zd, max length supported: %d", len, MAX_ID_LEN);
    alarmMgr.badInput(clientIp, errorMsg);
    return std::string(errorMsg);
  }

  if ((len = strlen(entityId.typePattern.c_str())) < MIN_ID_LEN)
  {
    snprintf(errorMsg, sizeof errorMsg, "entity typePattern length: %zd, min length supported: %d", len, MIN_ID_LEN);
    alarmMgr.badInput(clientIp, errorMsg);
    return std::string(errorMsg);
  }

  return "OK";
}



/* ****************************************************************************
*
* ContextElement::check
*
*/
std::string Entity::check(RequestType requestType)
{
  std::string err;
  char errorMsg[128];

  // In EntityRequest the entity id comes in the URL so Entity is allowed to have empty id and idPattern
  if ((requestType != EntityRequest) && (entityId.id.empty()) && (entityId.idPattern.empty()))
  {
    snprintf(errorMsg, sizeof errorMsg, "id and idPattern cannot be both empty at the same time");
    alarmMgr.badInput(clientIp, errorMsg);
    return std::string(errorMsg);
  }

  if ((!entityId.id.empty()) && (!entityId.idPattern.empty()))
  {
    snprintf(errorMsg, sizeof errorMsg, "id and idPattern cannot be both filled at the same time");
    alarmMgr.badInput(clientIp, errorMsg);
    return std::string(errorMsg);
  }

  if ((!entityId.id.empty()) && ((err = checkId(requestType)) != "OK"))
  {
    return err;
  }
  if ((!entityId.idPattern.empty()) && ((err = checkIdPattern(requestType)) != "OK"))
  {
    return err;
  }

  if ((entityId.type.empty()) && (entityId.typePattern.empty()))
  {
    snprintf(errorMsg, sizeof errorMsg, "type and typePattern cannot be both empty at the same time");
    alarmMgr.badInput(clientIp, errorMsg);
    return std::string(errorMsg);
  }

  if ((!entityId.type.empty()) && (!entityId.typePattern.empty()))
  {
    snprintf(errorMsg, sizeof errorMsg, "type and typePattern cannot be both filled at the same time");
    alarmMgr.badInput(clientIp, errorMsg);
    return std::string(errorMsg);
  }

  if ((!entityId.type.empty()) && ((err = checkType(requestType)) != "OK"))
  {
    return err;
  }

  if ((!entityId.typePattern.empty()) && ((err = checkTypePattern(requestType)) != "OK"))
  {
    return err;
  }

  return attributeVector.check(requestType);
}


/* ****************************************************************************
*
* Entity::fill - 
*/
void Entity::fill
(
  const EntityId&                _entityId,
  const ContextAttributeVector&  caV,
  double                         _creDate,
  double                         _modDate
)
{
  entityId   = _entityId;

  creDate    = _creDate;
  modDate    = _modDate;

  attributeVector.fill(caV);
}



/* ****************************************************************************
*
* Entity::fill -
*/
void Entity::fill
(
  const EntityId&     _entityId,
  const std::string&  _servicePath,
  double              _creDate,
  double              _modDate
)
{
  entityId   = _entityId;

  servicePath = _servicePath;

  creDate     = _creDate;
  modDate     = _modDate;
}



/* ****************************************************************************
*
* Entity::fill -
*/
void Entity::fill(const EntityId& _entityId)
{
  entityId   = _entityId;
}



/* ****************************************************************************
*
* Entity::fill -
*
* This constructor was ported from old ContextElement class
*/
void Entity::fill(const Entity& en, bool useDefaultType, bool cloneCompounds)
{
  entityId      = en.entityId;
  servicePath   = en.servicePath;
  creDate       = en.creDate;
  modDate       = en.modDate;

  if (useDefaultType && (entityId.type.empty()))
  {
    entityId.type = DEFAULT_ENTITY_TYPE;
  }

  attributeVector.fill(en.attributeVector, useDefaultType, cloneCompounds);

  providerList      = en.providerList;
  providerRegIdList = en.providerRegIdList;
}



/* ****************************************************************************
*
* Entity::fill -
*/
void Entity::fill(const QueryContextResponse& qcrs, OrionError* oeP)
{
  Entity* eP = NULL;

  if (qcrs.error.code == SccContextElementNotFound)
  {
    oeP->fill(SccContextElementNotFound, ERROR_DESC_NOT_FOUND_ENTITY, ERROR_NOT_FOUND);
  }
  else if (qcrs.error.code != SccOk)
  {
    //
    // any other error distinct from Not Found
    //
    oeP->fill(qcrs.error.code, qcrs.error.description, qcrs.error.error);
  }
  else  // qcrs.error.code == SccOk
  {
    //
    // If there are more than one entity (ignoring SccContextElementNotFound cases), we return an error
    // Note SccContextElementNotFound could has been inserted by CPrs scenarios in some cases
    //
    for (unsigned int ix = 0; ix < qcrs.contextElementResponseVector.size(); ++ix)
    {
      if (qcrs.contextElementResponseVector[ix]->error.code != SccContextElementNotFound)
      {
        if (eP != NULL)
        {
          oeP->fill(SccConflict, ERROR_DESC_TOO_MANY_ENTITIES, ERROR_TOO_MANY);
          return;  // early return
        }
        else
        {
          eP = &qcrs.contextElementResponseVector[ix]->entity;
        }
      }
    }

    fill(eP->entityId,
         eP->attributeVector,
         eP->creDate,
         eP->modDate);
  }
}



/* ****************************************************************************
*
* Entity::applyUpdateOperators -
*/
void Entity::applyUpdateOperators(void)
{
  attributeVector.applyUpdateOperators();
}



/* ****************************************************************************
*
* Entity::release - 
*/
void Entity::release(void)
{
  attributeVector.release();
}



/* ****************************************************************************
*
* Entity::hideIdAndType
*
* Changes the attribute controlling if id and type are rendered in the JSON
*/
void Entity::hideIdAndType(bool hide)
{
  renderId = !hide;
}



/* ****************************************************************************
*
* Entity::getAttribute
*
* This constructor was ported from old ContextElement class
*/
ContextAttribute* Entity::getAttribute(const std::string& attrName)
{
  for (unsigned int ix = 0; ix < attributeVector.size(); ++ix)
  {
    ContextAttribute* caP = attributeVector[ix];

    if (dbEncode(caP->name) == attrName)
    {
      return caP;
    }
  }

  return NULL;
}



/* ****************************************************************************
*
* Entity::equal
*
* Same method as in EntityId class
*/
bool Entity::equal(Entity* eP)
{
  return entityId == eP->entityId;
}
