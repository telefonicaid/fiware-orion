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
* Entity::~Entity - 
*/
Entity::~Entity()
{
  release();
}


/* ****************************************************************************
*
* Entity::filterAttributes -
*
* Filter attributes vector in order to get the effective attribute vector to
* render.
*
* dateCreatedOption and dateModifiedOption are due to deprecated ways of requesting
* date in response. If used, the date is added at the end.
*
* FIXME P7: some comments regarding this function
*
* - Code should be integrated with the one on ContextElement::filterAttributes(). However,
*   this is probable a sub-task of the more general task of changing ContextElement to use
*   Entity class internally
* - The function is pretty long and complex. It should be refactored to simplify it
* - Related with this simplification maybe dateCreated and dateModified should be pre-included
*   in the attributes vector, so all three (dateCreated, dateModified and dateExpires) could be
*   processed in a similar way.
* - This function shouldn't be called from toJson() code. The mongoBackend should deal with
*   attributeVector preparation. Thus, either we leave the function heare and mongoBackend calls
*   Entity::filterAttributes() or the logic is moved to mongoBackend (to be decided when we face
*   this FIXME).
*/
void Entity::filterAttributes
(
  const std::vector<std::string>&  attrsFilter,
  bool                             dateCreatedOption,
  bool                             dateModifiedOption
)
{
  bool dateCreatedAdded  = false;
  bool dateModifiedAdded = false;

  if (attrsFilter.size () != 0)
  {
    if (std::find(attrsFilter.begin(), attrsFilter.end(), ALL_ATTRS) != attrsFilter.end())
    {
      // Not filtering, just adding dateCreated and dateModified if needed
      if ((creDate != 0) && (std::find(attrsFilter.begin(), attrsFilter.end(), DATE_CREATED) != attrsFilter.end()))
      {
        ContextAttribute* caP = new ContextAttribute(DATE_CREATED, DATE_TYPE, creDate);
        attributeVector.push_back(caP);
        dateCreatedAdded = true;
      }
      if ((modDate != 0) &&  (std::find(attrsFilter.begin(), attrsFilter.end(), DATE_MODIFIED) != attrsFilter.end()))
      {
        ContextAttribute* caP = new ContextAttribute(DATE_MODIFIED, DATE_TYPE, modDate);
        attributeVector.push_back(caP);
        dateModifiedAdded = true;
      }
    }
    else
    {
      // Reorder attributes in the same order they are in attrsFilter, excluding the ones
      // note there (i.e. filtering them out) and giving special treatment to creation
      // and modification dates
      //
      // The (attributeVector.lookup(DATE_XXXX) == -1) check is to give preference to user
      // defined attributes (see
      // https://fiware-orion.readthedocs.io/en/master/user/ngsiv2_implementation_notes/index.html#datemodified-and-datecreated-attributes)

      std::vector<ContextAttribute*> caNewV;

      for (unsigned int ix = 0; ix < attrsFilter.size(); ix++)
      {
        std::string attrsFilterItem = attrsFilter[ix];
        if ((creDate != 0) && (attrsFilterItem == DATE_CREATED) && (attributeVector.lookup(DATE_CREATED) == -1))
        {
          ContextAttribute* caP = new ContextAttribute(DATE_CREATED, DATE_TYPE, creDate);
          caNewV.push_back(caP);
          dateCreatedAdded = true;
        }
        else if ((modDate != 0) && (attrsFilterItem == DATE_MODIFIED) && (attributeVector.lookup(DATE_MODIFIED) == -1))
        {
          ContextAttribute* caP = new ContextAttribute(DATE_MODIFIED, DATE_TYPE, modDate);
          caNewV.push_back(caP);
          dateModifiedAdded = true;
        }
        // Actual attribute filtering only takes place if '*' was not used
        else
        {
          int found = attributeVector.lookup(attrsFilterItem);
          if (found != -1)
          {
            caNewV.push_back(attributeVector.vec[found]);
            attributeVector.vec.erase(attributeVector.vec.begin() + found);
          }
        }
      }

      // All the remainder elements in attributeVector need to be released,
      // before overriding the vector with caNewV
      attributeVector.release();

      attributeVector.vec = caNewV;
    }
  }

  // Legacy support for options=dateCreated and opations=dateModified
  if (dateCreatedOption && !dateCreatedAdded && (creDate != 0))
  {
    ContextAttribute* caP = new ContextAttribute(DATE_CREATED, DATE_TYPE, creDate);
    attributeVector.push_back(caP);
  }
  if (dateModifiedOption && !dateModifiedAdded && (modDate != 0))
  {
    ContextAttribute* caP = new ContextAttribute(DATE_MODIFIED, DATE_TYPE, modDate);
    attributeVector.push_back(caP);
  }

  // Removing dateExpires if not explictely included in the filter
  bool includeDateExpires = (std::find(attrsFilter.begin(), attrsFilter.end(), DATE_EXPIRES) != attrsFilter.end());
  int found;
  if (!includeDateExpires && ((found = attributeVector.lookup(DATE_EXPIRES)) != -1))
  {
    attributeVector.vec[found]->release();
    attributeVector.vec.erase(attributeVector.vec.begin() + found);
  }
}



/* ****************************************************************************
*
* Entity::render - 
*
* The rendering of JSON in APIv2 depends on the URI param 'options'
* Rendering methods:
*   o 'normalized' (default)
*   o 'keyValues'  (less verbose, only name and values shown for attributes - no type, no metadatas)
*   o 'values'     (only the values of the attributes are printed, in a vector)
*/
std::string Entity::render
(
  std::map<std::string, bool>&         uriParamOptions,
  std::map<std::string, std::string>&  uriParam
)
{
  if ((oe.details != "") || ((oe.reasonPhrase != "OK") && (oe.reasonPhrase != "")))
  {
    return oe.toJson();
  }

  RenderFormat  renderFormat = NGSI_V2_NORMALIZED;

  if      (uriParamOptions[OPT_KEY_VALUES]    == true)  { renderFormat = NGSI_V2_KEYVALUES;     }
  else if (uriParamOptions[OPT_VALUES]        == true)  { renderFormat = NGSI_V2_VALUES;        }
  else if (uriParamOptions[OPT_UNIQUE_VALUES] == true)  { renderFormat = NGSI_V2_UNIQUE_VALUES; }

  std::vector<std::string>  metadataFilter;
  std::vector<std::string>  attrsFilter;

  if (uriParam[URI_PARAM_METADATA] != "")
  {
    stringSplit(uriParam[URI_PARAM_METADATA], ',', metadataFilter);
  }

  if (uriParam[URI_PARAM_ATTRS] != "")
  {
    stringSplit(uriParam[URI_PARAM_ATTRS], ',', attrsFilter);
  }

  // Get the effective vector of attributes to render
  // FIXME P7: filterAttributes will be moved and invoking it would not be needed from toJson()
  // (see comment in filterAttributes)
  filterAttributes(attrsFilter, uriParamOptions[DATE_CREATED], uriParamOptions[DATE_MODIFIED]);

  std::string out;
  switch (renderFormat)
  {
  case NGSI_V2_VALUES:
    out = toJsonValues();
    break;
  case NGSI_V2_UNIQUE_VALUES:
    out = toJsonUniqueValues();
    break;
  case NGSI_V2_KEYVALUES:
    out = toJsonKeyvalues();
    break;
  default:  // NGSI_V2_NORMALIZED
    out = toJsonNormalized(metadataFilter);
    break;
  }

  return out;
}



/* ****************************************************************************
*
* Entity::toJsonValues -
*/
std::string Entity::toJsonValues(void)
{
  std::string out = "[";

  for (unsigned int ix = 0; ix < attributeVector.size(); ix++)
  {
    ContextAttribute* caP = attributeVector[ix];
    out += caP->toJsonValue();

    if (ix != attributeVector.size() - 1)
    {
      out += ",";
    }
  }

  out += "]";

  return out;
}



/* ****************************************************************************
*
* Entity::toJsonUniqueValues -
*/
std::string Entity::toJsonUniqueValues(void)
{
  std::string out = "[";

  std::map<std::string, bool>  uniqueMap;

  for (unsigned int ix = 0; ix < attributeVector.size(); ix++)
  {
    ContextAttribute* caP = attributeVector[ix];

    std::string value = caP->toJsonValue();

    if (uniqueMap[value] == true)
    {
      // Already rendered. Skip.
      continue;
    }
    else
    {
      out += value;
      uniqueMap[value] = true;
    }

    out += ",";
  }

  // The substrig trick removes the final ",". It is not very smart, but it saves
  // a second pass on the vector, once the "unicity" has been calculated in the hashmap
  return out.substr(0, out.length() - 1 ) + "]";
}



/* ****************************************************************************
*
* Entity::toJsonKeyvalues -
*/
std::string Entity::toJsonKeyvalues(void)
{
  JsonHelper jh;

  if (renderId)
  {
    jh.addString("id", id);

    /* This is needed for entities coming from NGSIv1 (which allows empty or missing types) */
    jh.addString("type", (type != "")? type : DEFAULT_ENTITY_TYPE);
  }

  for (unsigned int ix = 0; ix < attributeVector.size(); ix++)
  {
    ContextAttribute* caP = attributeVector[ix];
    jh.addRaw(caP->name, caP->toJsonValue());
  }

  return jh.str();
}



/* ****************************************************************************
*
* Entity::toJsonNormalized -
*/
std::string Entity::toJsonNormalized(const std::vector<std::string>&  metadataFilter)
{
  JsonHelper jh;

  if (renderId)
  {
    jh.addString("id", id);

    /* This is needed for entities coming from NGSIv1 (which allows empty or missing types) */
    jh.addString("type", (type != "")? type : DEFAULT_ENTITY_TYPE);
  }

  for (unsigned int ix = 0; ix < attributeVector.size(); ix++)
  {
    ContextAttribute* caP = attributeVector[ix];
    jh.addRaw(caP->name, caP->toJson(metadataFilter));
  }

  return jh.str();
}



/* ****************************************************************************
*
* Entity::check - 
*/
std::string Entity::check(RequestType requestType)
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

  return attributeVector.check(V2, requestType);
}



/* ****************************************************************************
*
* Entity::fill - 
*/
void Entity::fill
(
  const std::string&       _id,
  const std::string&       _type,
  const std::string&       _isPattern,
  ContextAttributeVector*  aVec,
  double                   _creDate,
  double                   _modDate
)
{
  id         = _id;
  type       = _type;
  isPattern  = _isPattern;

  creDate    = _creDate;
  modDate    = _modDate;

  attributeVector.fill(aVec);
}



/* ****************************************************************************
*
* Entity::fill -
*/
void Entity::fill(QueryContextResponse* qcrsP)
{
  if (qcrsP->errorCode.code == SccContextElementNotFound)
  {
    oe.fill(SccContextElementNotFound, ERROR_DESC_NOT_FOUND_ENTITY, ERROR_NOT_FOUND);
  }
  else if (qcrsP->errorCode.code != SccOk)
  {
    //
    // any other error distinct from Not Found
    //
    oe.fill(qcrsP->errorCode.code, qcrsP->errorCode.details, qcrsP->errorCode.reasonPhrase);
  }
  else if (qcrsP->contextElementResponseVector.size() > 1)  // qcrsP->errorCode.code == SccOk
  {
    //
    // If there are more than one entity, we return an error
    //
    oe.fill(SccConflict, ERROR_DESC_TOO_MANY_ENTITIES, ERROR_TOO_MANY);
  }
  else
  {
    ContextElement* ceP = &qcrsP->contextElementResponseVector[0]->contextElement;

    fill(ceP->entityId.id,
         ceP->entityId.type,
         ceP->entityId.isPattern,
         &ceP->contextAttributeVector,
         ceP->entityId.creDate,
         ceP->entityId.modDate);
  }
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
