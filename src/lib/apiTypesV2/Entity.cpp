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
#include "common/tag.h"
#include "common/string.h"
#include "common/globals.h"
#include "common/JsonHelper.h"
#include "common/errorMessages.h"
#include "rest/uriParamNames.h"
#include "alarmMgr/alarmMgr.h"
#include "parse/forbiddenChars.h"
#include "ngsi10/QueryContextResponse.h"
#include "mongoBackend/dbFieldEncoding.h"
#include "rest/OrionError.h"

#include "apiTypesV2/Entity.h"



/* ****************************************************************************
*
* Entity::Entity - 
*/
Entity::Entity(): isTypePattern(false), typeGiven(false), renderId(true), creDate(0), modDate(0)
{
}



/* ****************************************************************************
*
* Entity::Entity -
*
* This constructor was ported from old ContextElement class
*/
Entity::Entity(const std::string& _id, const std::string& _type, const std::string& _isPattern)
{
  id            = _id;
  type          = _type;
  isPattern     = _isPattern;
}



/* ****************************************************************************
*
* Entity::Entity -
*
* This constructor was ported from old ContextElement class
*/
Entity::Entity(EntityId* eP)
{
  id            = eP->id;
  type          = eP->type;
  isPattern     = eP->isPattern;
  isTypePattern = eP->isTypePattern;
  servicePath   = eP->servicePath;
  creDate       = eP->creDate;
  modDate       = eP->modDate;
}



/* ****************************************************************************
*
* Entity::Entity -
*
* This constructor was ported from old ContextElement class
*/
Entity::Entity(Entity* eP)
{
  fill(*eP);
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
* Entity::addAllAttrsExceptShadowed -
*
*/
void Entity::addAllAttrsExceptShadowed(std::vector<ContextAttribute*>*  orderedAttrs)
{
  for (unsigned int ix = 0; ix < attributeVector.size(); ix++)
  {
    if (!attributeVector[ix]->shadowed)
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
  std::vector<ContextAttribute*>*  orderedAttrs)
{
  if (blacklist)
  {
    if (attrsFilter.size() == 0)
    {
      // No filter, no blacklist. Attributes are "as is" in the entity except shadowed ones,
      // which require explicit inclusion (dateCreated, etc.)
      addAllAttrsExceptShadowed(orderedAttrs);
    }
    else
    {
      // Filter, blacklist. The order is the one in the entity, after removing attributes.
      // In blacklist case shadowed attributes (dateCreated, etc) are never included
      for (unsigned int ix = 0; ix < attributeVector.size(); ix++)
      {
        std::string name = attributeVector[ix]->name;
        if ((!attributeVector[ix]->shadowed) && (std::find(attrsFilter.begin(), attrsFilter.end(), name) == attrsFilter.end()))
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
      // except shadowed ones (dateCreated, etc.)
      addAllAttrsExceptShadowed(orderedAttrs);
    }
    else
    {
      // Filter, no blacklist. Processing will depend on whether '*' is in the attrsFilter or not
      if (std::find(attrsFilter.begin(), attrsFilter.end(), ALL_ATTRS) != attrsFilter.end())
      {
        // - If '*' is in: all attributes are included in the same order used by the entity
        for (unsigned int ix = 0; ix < attributeVector.size(); ix++)
        {
          if (attributeVector[ix]->shadowed)
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
        // FIXME #3168: replace getAll() by get() once metadata ID gets removed
        for (unsigned int ix = 0; ix < attrsFilter.size(); ix++)
        {
          std::vector<int> found;
          attributeVector.getAll(attrsFilter[ix], &found);
          for (unsigned int jx = 0; jx < found.size(); jx++)
          {
            orderedAttrs->push_back(attributeVector[found[jx]]);
          }
        }
      }
    }
  }
}




/* ****************************************************************************
*
* Entity::toJsonV1 -
*
* This method was ported from old ContextElement class. It was name render() there
*
*/
std::string Entity::toJsonV1
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
  std::string  out                              = "";
  bool         contextAttributeVectorRendered   = attributeVector.size() != 0;

  out += startTag(requestType != UpdateContext? "contextElement" : "");

  // Filter and order attributes
  std::vector<ContextAttribute*> orderedAttrs;
  filterAndOrderAttrs(attrsFilter, blacklist, &orderedAttrs);

  EntityId en(id, type, isPattern);
  out += en.toJsonV1(contextAttributeVectorRendered, false);
  out += attributeVector.toJsonV1(asJsonObject, requestType, orderedAttrs, metadataFilter, false, omitAttributeValues);

  out += endTag(comma, false);

  return out;
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
*/
std::string Entity::toJson
(
  RenderFormat                     renderFormat,
  const std::vector<std::string>&  attrsFilter,
  bool                             blacklist,
  const std::vector<std::string>&  metadataFilter
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
    out = toJsonUniqueValues(orderedAttrs);
    break;
  case NGSI_V2_KEYVALUES:
    out = toJsonKeyvalues(orderedAttrs);
    break;
  default:  // NGSI_V2_NORMALIZED
    out = toJsonNormalized(orderedAttrs, metadataFilter);
    break;
  }

  return out;
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
    jh.addString("id", id);

    /* This is needed for entities coming from NGSIv1 (which allows empty or missing types) */
    jh.addString("type", (type != "")? type : DEFAULT_ENTITY_TYPE);
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
*/
std::string Entity::toJsonNormalized(const std::vector<ContextAttribute*>& orderedAttrs, const std::vector<std::string>&  metadataFilter)
{
  JsonObjectHelper jh;

  if (renderId)
  {
    jh.addString("id", id);

    /* This is needed for entities coming from NGSIv1 (which allows empty or missing types) */
    jh.addString("type", (type != "")? type : DEFAULT_ENTITY_TYPE);
  }

  for (unsigned int ix = 0; ix < orderedAttrs.size(); ix++)
  {
    ContextAttribute* caP = orderedAttrs[ix];
    jh.addRaw(caP->name, caP->toJson(metadataFilter));
  }

  return jh.str();
}



/* ****************************************************************************
*
* toString -
*
* FIXME P3: Copied from EntityId class
*/
std::string Entity::toString(bool useIsPattern, const std::string& delimiter)
{
  std::string s;

  s = id + delimiter + type;

  if (useIsPattern)
  {
    s += delimiter + isPattern;
  }

  return s;
}



/* ****************************************************************************
*
* ContextElement::check
*
* This V1 "branch" of this method has been ported from old ContextElement class
*
*/
std::string Entity::check(ApiVersion apiVersion, RequestType requestType)
{
  if (apiVersion == V1)
  {
    std::string res;

    if (id == "")
    {
      return "empty entityId:id";
    }

    if (!isTrue(isPattern) && !isFalse(isPattern) && isPattern != "")
    {
      return std::string("invalid isPattern value for entity: /") + isPattern + "/";
    }

    if ((requestType == RegisterContext) && (isTrue(isPattern)))
    {
      return "isPattern set to true for registrations is currently not supported";
    }

    if (isTrue(isPattern))
    {
      regex_t re;
      if ((id.find('\0') != std::string::npos) || (regcomp(&re, id.c_str(), REG_EXTENDED) != 0))
      {
        return "invalid regex for entity id pattern";
      }
      regfree(&re);  // If regcomp fails it frees up itself (see glibc sources for details)
    }
  }
  else  // V2
  {
    ssize_t  len;
    char     errorMsg[128];

    if (((len = strlen(id.c_str())) < MIN_ID_LEN) && (requestType != EntityRequest))
    {
      snprintf(errorMsg, sizeof errorMsg, "entity id length: %zd, min length supported: %d", len, MIN_ID_LEN);
      alarmMgr.badInput(clientIp, errorMsg);
      return std::string(errorMsg);
    }

    if ((requestType == EntitiesRequest) && (id.empty()))
    {
      return "No Entity ID";
    }

    if ( (len = strlen(id.c_str())) > MAX_ID_LEN)
    {
      snprintf(errorMsg, sizeof errorMsg, "entity id length: %zd, max length supported: %d", len, MAX_ID_LEN);
      alarmMgr.badInput(clientIp, errorMsg);
      return std::string(errorMsg);
    }

    if (isPattern.empty())
    {
      isPattern = "false";
    }

    // isPattern MUST be either "true" or "false" (or empty => "false")
    if ((isPattern != "true") && (isPattern != "false"))
    {
      alarmMgr.badInput(clientIp, "invalid value for isPattern");
      return "Invalid value for isPattern";
    }

    // Check for forbidden chars for "id", but not if "id" is a pattern
    if (isPattern == "false")
    {
      if (forbiddenIdChars(V2, id.c_str()))
      {
        alarmMgr.badInput(clientIp, ERROR_DESC_BAD_REQUEST_INVALID_CHAR_ENTID);
        return ERROR_DESC_BAD_REQUEST_INVALID_CHAR_ENTID;
      }
    }

    if ( (len = strlen(type.c_str())) > MAX_ID_LEN)
    {
      snprintf(errorMsg, sizeof errorMsg, "entity type length: %zd, max length supported: %d", len, MAX_ID_LEN);
      alarmMgr.badInput(clientIp, errorMsg);
      return std::string(errorMsg);
    }

    if (!((requestType == BatchQueryRequest) || (requestType == BatchUpdateRequest && !typeGiven)))
    {
      if ( (len = strlen(type.c_str())) < MIN_ID_LEN)
      {
        snprintf(errorMsg, sizeof errorMsg, "entity type length: %zd, min length supported: %d", len, MIN_ID_LEN);
        alarmMgr.badInput(clientIp, errorMsg);
        return std::string(errorMsg);
      }
    }

    // Check for forbidden chars for "type", but not if "type" is a pattern
    if (isTypePattern == false)
    {
      if (forbiddenIdChars(V2, type.c_str()))
      {
        alarmMgr.badInput(clientIp, ERROR_DESC_BAD_REQUEST_INVALID_CHAR_ENTTYPE);
        return ERROR_DESC_BAD_REQUEST_INVALID_CHAR_ENTTYPE;
      }
    }
  }

  // Common part (V1 and V2)
  return attributeVector.check(apiVersion, requestType);
}


/* ****************************************************************************
*
* Entity::fill - 
*/
void Entity::fill
(
  const std::string&             _id,
  const std::string&             _type,
  const std::string&             _isPattern,
  const ContextAttributeVector&  caV,
  double                         _creDate,
  double                         _modDate
)
{
  id         = _id;
  type       = _type;
  isPattern  = _isPattern;

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
  const std::string&  _id,
  const std::string&  _type,
  const std::string&  _isPattern,
  const std::string&  _servicePath,
  double              _creDate,
  double              _modDate
)
{
  id          = _id;
  type        = _type;
  isPattern   = _isPattern;

  servicePath = _servicePath;

  creDate     = _creDate;
  modDate     = _modDate;
}



/* ****************************************************************************
*
* Entity::fill -
*/
void Entity::fill
(
  const std::string&  _id,
  const std::string&  _type,
  const std::string&  _isPattern
)
{
  id         = _id;
  type       = _type;
  isPattern  = _isPattern;
}



/* ****************************************************************************
*
* Entity::fill -
*
* This constructor was ported from old ContextElement class
*/
void Entity::fill(const Entity& en, bool useDefaultType, bool cloneCompounds)
{
  id            = en.id;
  type          = en.type;
  isPattern     = en.isPattern;
  isTypePattern = en.isTypePattern;
  servicePath   = en.servicePath;
  creDate       = en.creDate;
  modDate       = en.modDate;

  if (useDefaultType && (type == ""))
  {
    type = DEFAULT_ENTITY_TYPE;
  }

  attributeVector.fill(en.attributeVector, useDefaultType, cloneCompounds);

  providingApplicationList = en.providingApplicationList;
}



/* ****************************************************************************
*
* Entity::fill -
*/
void Entity::fill(const QueryContextResponse& qcrs, OrionError* oeP)
{
  if (qcrs.errorCode.code == SccContextElementNotFound)
  {
    oeP->fill(SccContextElementNotFound, ERROR_DESC_NOT_FOUND_ENTITY, ERROR_NOT_FOUND);
  }
  else if (qcrs.errorCode.code != SccOk)
  {
    //
    // any other error distinct from Not Found
    //
    oeP->fill(qcrs.errorCode.code, qcrs.errorCode.details, qcrs.errorCode.reasonPhrase);
  }
  else if (qcrs.contextElementResponseVector.size() > 1)  // qcrs.errorCode.code == SccOk
  {
    //
    // If there are more than one entity, we return an error
    //
    oeP->fill(SccConflict, ERROR_DESC_TOO_MANY_ENTITIES, ERROR_TOO_MANY);
  }
  else
  {
    Entity* eP = &qcrs.contextElementResponseVector[0]->entity;

    fill(eP->id,
         eP->type,
         eP->isPattern,
         eP->attributeVector,
         eP->creDate,
         eP->modDate);
  }
}



/* ****************************************************************************
*
* Entity::release - 
*/
void Entity::release(bool skipCompounds)
{
  attributeVector.release(skipCompounds);
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

    if (dbDotEncode(caP->name) == attrName)
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
* Same method that in EntityId class
*/
bool Entity::equal(Entity* eP)
{
  return ((eP->id                == id)                &&
          (eP->type              == type)              &&
          (isTrue(eP->isPattern) == isTrue(isPattern)) &&
          (eP->isTypePattern     == isTypePattern));
}
