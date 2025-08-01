/*
*
* Copyright 2016 Telefonica Investigacion y Desarrollo, S.A.U
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

#include "rapidjson/document.h"

#include "common/string.h"
#include "common/errorMessages.h"
#include "rest/ConnectionInfo.h"
#include "ngsi/ParseData.h"
#include "ngsi/Request.h"
#include "parse/forbiddenChars.h"

#include "jsonParseV2/jsonParseTypeNames.h"
#include "jsonParseV2/parseContextAttribute.h"
#include "jsonParseV2/parseEntityObject.h"



/* ****************************************************************************
*
* parseEntityObject -
*/
std::string parseEntityObject
(
  ConnectionInfo*                        ciP,
  rapidjson::Value::ConstValueIterator&  valueP,
  Entity*                                eP,
  bool                                   idPatternAllowed,
  bool                                   attrsAllowed
)
{
  std::string type  = jsonParseTypeNames[valueP->GetType()];

  if (type != "Object")
  {
    return "entity is not a JSON object";
  }

  if (valueP->HasMember("id") && valueP->HasMember("idPattern"))
  {
    return "both /id/ and /idPattern/ present";
  }

  if (!valueP->HasMember("id") && !valueP->HasMember("idPattern"))
  {
    return "nor /id/ nor /idPattern/ present";
  }

  bool typeGiven = false;
  for (rapidjson::Value::ConstMemberIterator iter = valueP->MemberBegin(); iter != valueP->MemberEnd(); ++iter)
  {
    std::string name  = iter->name.GetString();
    std::string type  = jsonParseTypeNames[iter->value.GetType()];

    if (name == "id")
    {
      if (type != "String")
      {
        return ERROR_DESC_BAD_REQUEST_INVALID_JTYPE_ENTID;
      }

      eP->entityId.id = iter->value.GetString();

      if (forbiddenIdChars(eP->entityId.id.c_str(), ""))
      {
        return ERROR_DESC_BAD_REQUEST_INVALID_CHAR_ENTID;
      }
    }
    else if (name == "idPattern")
    {
      if (idPatternAllowed == false)
      {
        return ERROR_NOTIMPLEMENTED;
      }
      if (type != "String")
      {
        return ERROR_DESC_BAD_REQUEST_INVALID_JTYPE_ENTIDPATTERN;
      }

      regex_t re;
      if (!regComp(&re, iter->value.GetString(), REG_EXTENDED))
      {
        return ERROR_DESC_BAD_REQUEST_INVALID_REGEX_ENTIDPATTERN;
      }
      regfree(&re);  // If regcomp fails it frees up itself (see glibc sources for details)

      eP->entityId.idPattern = iter->value.GetString();
    }
    else if (name == "type")
    {
      if (type != "String")
      {
        return ERROR_DESC_BAD_REQUEST_INVALID_JTYPE_ENTTYPE;
      }

      eP->entityId.type  = iter->value.GetString();
      typeGiven          = true;

      if (eP->entityId.type.empty())
      {
        return ERROR_DESC_BAD_REQUEST_EMPTY_ENTTYPE;
      }

      if (forbiddenIdChars(eP->entityId.type.c_str(), ""))
      {
        return ERROR_DESC_BAD_REQUEST_INVALID_CHAR_ENTTYPE;
      }
    }
    else if (name == "typePattern")
    {
      if (type != "String")
      {
        return ERROR_DESC_BAD_REQUEST_INVALID_JTYPE_ENTTYPEPATTERN;
      }

      regex_t re;
      if (!regComp(&re, iter->value.GetString(), REG_EXTENDED))
      {
        return ERROR_DESC_BAD_REQUEST_INVALID_REGEX_ENTTYPEPATTERN;
      }
      regfree(&re);  // If regcomp fails it frees up itself (see glibc sources for details)

      eP->entityId.typePattern  = iter->value.GetString();
      typeGiven          = true;
    }
    else
    {
      std::string r;

      if (!attrsAllowed)
      {
        return "no attributes allowed in Entity in this payload";
      }

      ContextAttribute* caP = new ContextAttribute();

      r = parseContextAttribute(ciP, iter, caP, true);
      if (r == "OK")
      {
        eP->attributeVector.push_back(caP);
      }
      else
      {
        delete caP;
        return r;
      }
    }
  }

  if (!typeGiven)
  {
    eP->entityId.typePattern = ".*";
  }

  return eP->check(ciP->requestType);
}
