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
    eP->errorCode.fill("JSON Parse Error", "");
    return eP->render(ciP, EntitiesRequest);
  }


  if (!document.IsObject())
  {
    LM_E(("Bad Input (JSON Parse Error)"));
    eP->errorCode.fill("JSON Parse Error", "");
    return eP->render(ciP, EntitiesRequest);
  }


  if (document.HasMember("id") && document.HasMember("idPattern"))
  {
    LM_W(("Bad Input (both 'id' and 'idPattern' specified"));
    eP->errorCode.fill("JSON Parse Error", "both 'id'and 'idPattern'specified");
    return eP->render(ciP, EntitiesRequest);
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
        eP->errorCode.fill("JSON Parse Error", "bad type for EntityId::id");
        return eP->render(ciP, EntitiesRequest);
      }

      eP->id = iter->value.GetString();
    }
    else if (name == "type")
    {
      if (type != "String")
      {
        LM_W(("Bad Input (bad type for EntityId::type"));
        eP->errorCode.fill("JSON Parse Error", "bad type for EntityId::type");
        return eP->render(ciP, EntitiesRequest);
      }

      eP->type = iter->value.GetString();
    }
    else if (name == "idPattern")
    {
      if (type != "String")
      {
        LM_W(("Bad Input (bad type for EntityId::isPattern)"));
        eP->errorCode.fill("JSON Parse Error", "bad type for EntityId::isPattern");
        return eP->render(ciP, EntitiesRequest);
      }

      eP->isPattern = "true";
      eP->id        = iter->value.GetString();
    }
    else
    {
      ContextAttribute* caP = new ContextAttribute();
      eP->attributeVector.push_back(caP);

      std::string r = parseContextAttribute(iter, caP);
      if (r != "OK")
      {
        LM_W(("Bad Input (parse error in EntityId::ContextAttribute)"));
        eP->errorCode.fill("JSON Parse Error", r);
        return eP->render(ciP, EntitiesRequest);
      }
    }
  }

  std::string location = "/v2/entities/" + eP->id;

  ciP->httpHeader.push_back("Location");
  ciP->httpHeaderValue.push_back(location);
  ciP->httpStatusCode = SccCreated;

  return "OK";
}
