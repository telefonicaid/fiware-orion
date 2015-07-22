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
#include "jsonParseV2/jsonParseTypeNames.h"
#include "jsonParseV2/parseEntity.h"
#include "jsonParseV2/parseContextAttribute.h"

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
    eP->errorCode.fill("ParseError", "Errors found in incoming JSON buffer");
    ciP->httpStatusCode = SccBadRequest;;
    return eP->render(ciP, EntitiesRequest);
  }


  if (!document.IsObject())
  {
    LM_E(("Bad Input (JSON Parse Error)"));
    eP->errorCode.fill("ParseError", "Error parsing incoming JSON buffer");
    ciP->httpStatusCode = SccBadRequest;;
    return eP->render(ciP, EntitiesRequest);
  }


  if (!document.HasMember("id"))
  {
    LM_W(("Bad Input (No entity id specified"));
    eP->errorCode.fill("ParseError", "no entity id specified");
    ciP->httpStatusCode = SccBadRequest;;
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
        LM_W(("Bad Input (invalid JSON type for entity id"));
        eP->errorCode.fill("ParseError", "invalid JSON type for entity id");
        ciP->httpStatusCode = SccBadRequest;;
        return eP->render(ciP, EntitiesRequest);
      }

      eP->id = iter->value.GetString();
    }
    else if (name == "type")
    {
      if (type != "String")
      {
        LM_W(("Bad Input (invalid JSON type for entity type"));
        eP->errorCode.fill("ParseError", "invalid JSON type for entity type");
        ciP->httpStatusCode = SccBadRequest;;
        return eP->render(ciP, EntitiesRequest);
      }

      eP->type = iter->value.GetString();
    }
    else
    {
      ContextAttribute* caP = new ContextAttribute();
      eP->attributeVector.push_back(caP);

      std::string r = parseContextAttribute(ciP, iter, caP);
      if (r != "OK")
      {
        LM_W(("Bad Input (parse error in context attribute)"));
        eP->errorCode.fill("ParseError", r);
        ciP->httpStatusCode = SccBadRequest;
        return eP->render(ciP, EntitiesRequest);
      }
    }
  }

  if (eP->id == "")
  {
    LM_W(("Bad Input (empty entity id"));
    eP->errorCode.fill("ParseError", "empty entity id");
    ciP->httpStatusCode = SccBadRequest;;
    return eP->render(ciP, EntitiesRequest);
  }

  LM_M(("Done parsing Entity"));
  eP->present("");
  return "OK";
}
