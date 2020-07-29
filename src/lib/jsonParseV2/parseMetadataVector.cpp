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

#include "rapidjson/document.h"

#include "logMsg/logMsg.h"

#include "ngsi/ContextAttribute.h"

#include "jsonParseV2/jsonParseTypeNames.h"
#include "jsonParseV2/parseMetadata.h"
#include "jsonParseV2/parseMetadataVector.h"



/* ****************************************************************************
*
* parseMetadataVector - 
*/
std::string parseMetadataVector(const rapidjson::Value::ConstMemberIterator& node, ContextAttribute* caP)
{
  std::string type   = jsonParseTypeNames[node->value.GetType()];

  if (type != "Object")
  {
    return "metadata not a JSON object";
  }

  for (rapidjson::Value::ConstMemberIterator iter = node->value.MemberBegin(); iter != node->value.MemberEnd(); ++iter)
  {
    std::string  r;
    Metadata*    mP = new Metadata();
    mP->name = iter->name.GetString();
    caP->metadataVector.push_back(mP);
    LM_TMP(("LEAK: Creating metadata '%s' for Attribute '%s'", mP->name.c_str(), caP->name.c_str()));
    r = parseMetadata(iter->value, mP);
    if (r != "OK")
    {
      LM_TMP(("LEAK: parseMetadata failed, NOT deleting the allocated metadata"));
      return r;
    }
    else
      LM_TMP(("LEAK: parseMetadata OK"));
  }

  return "OK";
}
