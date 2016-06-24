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

#include "logMsg/traceLevels.h"
#include "common/tag.h"
#include "common/string.h"
#include "common/errorMessages.h"
#include "alarmMgr/alarmMgr.h"
#include "parse/forbiddenChars.h"
#include "apiTypesV2/Entity.h"
#include "ngsi10/QueryContextResponse.h"



/* ****************************************************************************
*
* Entity::Entity - 
*/
Entity::Entity(): typeGiven(false), renderId(true)
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
* Entity::render - 
*
* The rendering of JSON in APIv2 depends on the URI param 'options'
* Rendering methods:
*   o 'normalized' (default)
*   o 'keyValues'  (less verbose, only name and values shown for attributes - no type, no metadatas)
*   o 'values'     (only the values of the attributes are printed, in a vector)
*/
std::string Entity::render(ConnectionInfo* ciP, RequestType requestType, bool comma)
{
  RenderFormat  renderFormat = NGSI_V2_NORMALIZED;

  if      (ciP->uriParamOptions[OPT_KEY_VALUES]    == true)  { renderFormat = NGSI_V2_KEYVALUES;     }
  else if (ciP->uriParamOptions[OPT_VALUES]        == true)  { renderFormat = NGSI_V2_VALUES;        }
  else if (ciP->uriParamOptions[OPT_UNIQUE_VALUES] == true)  { renderFormat = NGSI_V2_UNIQUE_VALUES; }

  if ((oe.details == "") && ((oe.reasonPhrase == "OK") || (oe.reasonPhrase == "")))
  {
    std::string out;

    if ((renderFormat == NGSI_V2_VALUES) || (renderFormat == NGSI_V2_UNIQUE_VALUES))
    {
      out = "[";
      if (attributeVector.size() != 0)
      {
        std::vector<std::string> attrsFilter;
        stringSplit(ciP->uriParam["attrs"], ',', attrsFilter);
        out += attributeVector.toJson(true, renderFormat, attrsFilter);
      }
      out += "]";        
    }
    else
    {
      out = "{";

      if (renderId)
      {
        out += JSON_VALUE("id", id);
        out += ",";

        /* This is needed for entities coming from NGSIv1 (which allows empty or missing types) */
        out += JSON_STR("type") + ":" + ((type != "")? JSON_STR(type) : JSON_STR(DEFAULT_TYPE));
      }

      std::string attrsOut;
      if (attributeVector.size() != 0)
      {
        std::vector<std::string> attrsFilter;
        stringSplit(ciP->uriParam["attrs"], ',', attrsFilter);

        attrsOut += attributeVector.toJson(true, renderFormat, attrsFilter);
      }

      // Note that just attributeVector.size() != 0 (used in previous versions) cannot be used
      // as ciP->uriParam["attrs"] filter could remove all the attributes
      if (attrsOut != "")
      {
        if (renderId)
        {
          out +=  "," + attrsOut;
        }
        else
        {
          out += attrsOut;
        }
      }

      out += "}";
    }

    if (comma)
    {
      out += ",";
    }

    return out;
  }

  return oe.toJson();
}



/* ****************************************************************************
*
* Entity::check - 
*/
std::string Entity::check(ConnectionInfo* ciP, RequestType requestType)
{
  ssize_t len;
  char errorMsg[128];

  if ((requestType == EntitiesRequest) && (id == ""))
  {
    return "No Entity ID";
  }

  if ( (len = strlen(id.c_str())) > MAX_ID_LEN)
  {
    snprintf(errorMsg, sizeof errorMsg, "entity id length: %zd, max length supported: %d", len, MAX_ID_LEN);
    alarmMgr.badInput(clientIp, errorMsg);
    return std::string(errorMsg);
  }

  if (forbiddenIdChars(ciP->apiVersion, id.c_str()))
  {
    alarmMgr.badInput(clientIp, "found a forbidden character in the id of an entity");
    return "Invalid characters in entity id";
  }

  if ( (len = strlen(type.c_str())) > MAX_ID_LEN)
  {
    snprintf(errorMsg, sizeof errorMsg, "entity type length: %zd, max length supported: %d", len, MAX_ID_LEN);
    alarmMgr.badInput(clientIp, errorMsg);
    return std::string(errorMsg);
  }

  if (ciP->apiVersion == "v2" && (len = strlen(type.c_str())) < MIN_ID_LEN)
  {
    snprintf(errorMsg, sizeof errorMsg, "entity type length: %zd, min length supported: %d", len, MIN_ID_LEN);
    alarmMgr.badInput(clientIp, errorMsg);
    return std::string(errorMsg);
  }

  if (forbiddenIdChars(ciP->apiVersion, type.c_str()))
  {
    alarmMgr.badInput(clientIp, "found a forbidden character in the type of an entity");
    return "Invalid characters in entity type";
  }

  if (forbiddenChars(isPattern.c_str()))
  {
    alarmMgr.badInput(clientIp, "found a forbidden character in the pattern of an entity");
    return "Invalid characters in entity isPattern";
  }

  return attributeVector.check(ciP, requestType, "", "", 0);
}



/* ****************************************************************************
*
* Entity::present - 
*/
void Entity::present(const std::string& indent)
{
  LM_T(LmtPresent, ("%sid:        %s", 
		    indent.c_str(), 
		    id.c_str()));
  LM_T(LmtPresent, ("%stype:      %s", 
		    indent.c_str(), 
		    type.c_str()));
  LM_T(LmtPresent, ("%sisPattern: %s", 
		    indent.c_str(), 
		    isPattern.c_str()));

  attributeVector.present(indent + "  ");
}



/* ****************************************************************************
*
* Entity::fill - 
*/
void Entity::fill(const std::string& _id, const std::string& _type, const std::string& _isPattern, ContextAttributeVector* aVec)
{
  id         = _id;
  type       = _type;
  isPattern  = _isPattern;

  attributeVector.fill(aVec);
}

void Entity::fill(QueryContextResponse* qcrsP)
{

  if (qcrsP->errorCode.code == SccContextElementNotFound)
  {
    oe.fill(SccContextElementNotFound, "The requested entity has not been found. Check type and id", "NotFound");
  }
  else if (qcrsP->errorCode.code != SccOk)
  {
    //
    // any other error distinct from Not Found
    //
    oe.fill(qcrsP->errorCode.code, qcrsP->errorCode.details, qcrsP->errorCode.reasonPhrase);
  }
  else if (qcrsP->contextElementResponseVector.size() > 1) // qcrsP->errorCode.code == SccOk
  {
      //
      // If there are more than one entity, we return an error
      //
      oe.fill(SccConflict, MORE_MATCHING_ENT, "TooManyResults");
  }
  else
  {
    ContextElement* ceP = &qcrsP->contextElementResponseVector[0]->contextElement;
    fill(ceP->entityId.id, ceP->entityId.type, ceP->entityId.isPattern, &ceP->contextAttributeVector);
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
*  Changes the attribute controlling if id and type
*  are rendered in the JSON
*/
void Entity::hideIdAndType(bool hide)
{
  renderId = !hide;
}
