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
#include "rapidjson/document.h"

#include "rest/ConnectionInfo.h"
#include "ngsi/ParseData.h"
#include "ngsi/Request.h"
#include "parse/forbiddenChars.h"
#include "jsonParseV2/jsonParseTypeNames.h"
#include "jsonParseV2/parseEntityObject.h"
#include "jsonParseV2/parseContextAttribute.h"

using namespace rapidjson;



/* ****************************************************************************
*
* parseEntityObject - 
*/
std::string parseEntityObject(ConnectionInfo* ciP, Value::ConstValueIterator valueP, Entity* eP, bool attrsAllowed)
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

  for (Value::ConstMemberIterator iter = valueP->MemberBegin(); iter != valueP->MemberEnd(); ++iter)
  {
    std::string name  = iter->name.GetString();
    std::string type  = jsonParseTypeNames[iter->value.GetType()];

    if (name == "id")
    {
      if (type != "String")
      {
        return "invalid JSON type for entity id";
      }

      eP->id = iter->value.GetString();
    }
    else if (name == "idPattern")
    {
      if (type != "String")
      {
        return "invalid JSON type for entity idPattern";
      }

      regex_t re;
      if (regcomp(&re, iter->value.GetString(), REG_EXTENDED) != 0)
      {
        return "invalid regex for entity id pattern";
      }
      regfree(&re);

      eP->id        = iter->value.GetString();
      eP->isPattern = "true";
    }
    else if (name == "type")
    {
      if (type != "String")
      {
        return "invalid JSON type for entity type";
      }

      eP->type      = iter->value.GetString();
      eP->typeGiven = true;
    }
    else if (name == "typePattern")
    {
      if (type != "String")
      {
        return "invalid JSON type for entity typePattern";
      }

      regex_t re;
      if (regcomp(&re, iter->value.GetString(), REG_EXTENDED) != 0)
      {
        return "invalid regex for entity type pattern";
      }
      regfree(&re);

      eP->type          = iter->value.GetString();
      eP->isTypePattern = true;
    }
    else
    {
      std::string r;

      if (!attrsAllowed)
      {
        return "no attributes allowed in Entity in this payload";
      }
      
      ContextAttribute* caP = new ContextAttribute();
      r = parseContextAttribute(ciP, iter, caP);
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

  return eP->check(ciP->apiVersion, ciP->requestType);
}
