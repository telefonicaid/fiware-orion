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
* Author: Orion dev team
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
* parseAttributes -
*/
std::string parseAttributes(ConnectionInfo* ciP, Entity* eP)
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


  for (Value::ConstMemberIterator iter = document.MemberBegin(); iter != document.MemberEnd(); ++iter)
  {
    std::string name   = iter->name.GetString();
    std::string type   = jsonParseTypeNames[iter->value.GetType()];

    if (name == "id")
    {
      LM_W(("Bad Input ('id' is not a valid attribute"));
      eP->errorCode.fill("ParseError", "invalid input, 'id' as attribute");
      ciP->httpStatusCode = SccBadRequest;;
      return eP->render(ciP, EntitiesRequest);
    }
    else if (name == "type")
    {
      LM_W(("Bad Input ('type'' is not a valid attribute"));
      eP->errorCode.fill("ParseError", "invalid input, 'type' as attribute");
      ciP->httpStatusCode = SccBadRequest;;
      return eP->render(ciP, EntitiesRequest);
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

  LM_M(("Done parsing Entity"));
  eP->present("");
  return "OK";
}
