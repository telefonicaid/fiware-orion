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
#include "rapidjson/document.h"

#include "rest/ConnectionInfo.h"
#include "ngsi/ParseData.h"
#include "ngsi/Request.h"
#include "parse/jsonParseTypeNames.h"
#include "parse/parseEntity.h"
#include "parse/parseContextAttribute.h"

using namespace rapidjson;



/* ****************************************************************************
*
* parseEntity - 
*/
std::string parseEntity(ConnectionInfo* ciP, Entity* eP)
{
  Document document;

  document.Parse(ciP->payload);

  if (document.HasParseError())
  {
    LM_W(("Bad Input (JSON parse error)"));
    return "parse error";
  }


  if (!document.IsObject())
  {
    LM_E(("JSON Parse Error"));
    return "Error";
  }


  for (Value::ConstMemberIterator iter = document.MemberBegin(); iter != document.MemberEnd(); ++iter)
  {
    std::string name   = iter->name.GetString();
    std::string type   = jsonParseTypeNames[iter->value.GetType()];

    LM_M(("KZ: %s (type: %s)", name.c_str(), type.c_str()));

    if (name == "id")
    {
      if (type != "String")
      {
        LM_W(("Bad Input (bad type for EntityId::id"));
        return "Parse Error";
      }

      eP->id = iter->value.GetString();
    }
    else if (name == "type")
    {
      if (type != "String")
      {
        LM_W(("Bad Input (bad type for EntityId::type"));
        return "Parse Error";
      }

      eP->type = iter->value.GetString();
    }
    else if (name == "isPattern")
    {
      if (type == "True")
      {
        eP->isPattern = "true";
      }
      else if (type == "False")
      {
        eP->isPattern = "false";
      }
      else if (type == "String")
      {
        eP->isPattern = iter->value.GetString();
      }
      else
      {
        LM_W(("Bad Input (bad type for EntityId::isPattern)"));
        return "Parse Error";
      }
    }
    else
    {
      ContextAttribute* caP = new ContextAttribute();
      eP->attributeVector.push_back(caP);

      std::string r = parseContextAttribute(iter, caP);
      if (r != "OK")
      {
        LM_W(("Bad Input (parse error in EntityId::ContextAttribute)"));
        return r;
      }
    }
  }

  eP->present("KZ");
  return "OK";
}
