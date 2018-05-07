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
#include <regex.h>

#include <string>
#include <vector>
#include <algorithm>

#include "rapidjson/document.h"

#include "alarmMgr/AlarmManager.h"
#include "alarmMgr/alarmMgr.h"
#include "rest/ConnectionInfo.h"
#include "rest/OrionError.h"
#include "parse/forbiddenChars.h"
#include "apiTypesV2/EntID.h"
#include "common/errorMessages.h"
#include "jsonParseV2/utilsParse.h"
#include "jsonParseV2/parseEntitiesVector.h"



/* ****************************************************************************
*
* parseEntitiesVector -
*/
bool parseEntitiesVector
(
  ConnectionInfo*              ciP,
  std::vector<ngsiv2::EntID>*  eivP,
  const rapidjson::Value&      entities,
  std::string*                 errorStringP
)
{
  if (!entities.IsArray())
  {
    *errorStringP = "/entities/ is not an array";
    return false;
  }

  for (rapidjson::Value::ConstValueIterator iter = entities.Begin(); iter != entities.End(); ++iter)
  {
    if (!iter->IsObject())
    {
      *errorStringP = "/entities/ vector item is not an object";
      return false;
    }

    if (!iter->HasMember("id") && !iter->HasMember("idPattern"))
    {
      *errorStringP = "/entities/ vector item does not have /id/ nor /idPattern/";
      return false;
    }

    if (iter->HasMember("id") && iter->HasMember("idPattern"))
    {
      *errorStringP = "/entities/ vector item has both /id/ and /idPattern/";
      return false;
    }

    if (iter->HasMember("type") && iter->HasMember("typePattern"))
    {
      *errorStringP = "/entities/ element has both /type/ and /typePattern/";
      return false;
    }


    std::string  id;
    std::string  idPattern;
    std::string  type;
    std::string  typePattern;

    {
      Opt<std::string> idOpt = getStringOpt(*iter, "id", "/entities/ vector item /id/");

      if (!idOpt.ok())
      {
        *errorStringP = idOpt.error;
        return false;
      }
      else if (idOpt.given)
      {
        if (idOpt.value.empty())
        {
          *errorStringP = "/entities/ element id is empty";
          return false;
        }
        if (forbiddenIdCharsV2(idOpt.value.c_str()))
        {
          *errorStringP = "forbidden characters in /entities/ vector item /id/";
          return false;
        }
        if (idOpt.value.length() > MAX_ID_LEN)
        {
          *errorStringP = "max id length exceeded in /entities/ vector item /id/";
          return false;
        }
        id = idOpt.value;
      }
    }

    {
      Opt<std::string> idPatOpt = getStringOpt(*iter, "idPattern", "/entities/ vector item /idPattern/");

      if (!idPatOpt.ok())
      {
        *errorStringP = idPatOpt.error;
        return false;
      }
      else if (idPatOpt.given)
      {
        if (idPatOpt.value.empty())
        {
          *errorStringP = "/entities/ vector item /idPattern/ is empty";
          return false;
        }

        idPattern = idPatOpt.value;

        // FIXME P5: Keep the regex and propagate to sub-cache
        regex_t re;
        if (regcomp(&re, idPattern.c_str(), REG_EXTENDED) != 0)
        {
          *errorStringP = ERROR_DESC_BAD_REQUEST_INVALID_REGEX_ENTIDPATTERN;
          return false;
        }
        regfree(&re);  // If regcomp fails it frees up itself
      }
    }

    {
      Opt<std::string> typeOpt = getStringOpt(*iter, "type", "/entities/ vector item type");

      if (!typeOpt.ok())
      {
        *errorStringP = typeOpt.error;
        return false;
      }
      else if (typeOpt.given)
      {
        if (forbiddenIdCharsV2(typeOpt.value.c_str()))
        {
          *errorStringP = "forbidden characters in /entities/ vector item /type/";
          return false;
        }
        if (typeOpt.value.length() > MAX_ID_LEN)
        {
          *errorStringP = "max type length exceeded in /entities/ vector item /type/";
          return false;
        }
        if (typeOpt.value.empty())
        {
          *errorStringP = ERROR_DESC_BAD_REQUEST_EMPTY_ENTTYPE;
          return false;
        }
        type = typeOpt.value;
      }
    }

    {
      Opt<std::string> typePatOpt = getStringOpt(*iter, "typePattern", "/entities/ vector item /typePattern/");

      if (!typePatOpt.ok())
      {
        *errorStringP = typePatOpt.error;
        return false;
      }
      else if (typePatOpt.given)
      {
        if (typePatOpt.value.empty())
        {
          *errorStringP = "/entities/ vector item /typePattern/ is empty";
          return false;
        }

        typePattern = typePatOpt.value;

        // FIXME P5: Keep the regex and propagate to sub-cache
        regex_t re;
        if (regcomp(&re, typePattern.c_str(), REG_EXTENDED) != 0)
        {
          *errorStringP = ERROR_DESC_BAD_REQUEST_INVALID_REGEX_ENTTYPEPATTERN;
          return false;
        }
        regfree(&re);  // If regcomp fails it frees up itself
      }
    }

    ngsiv2::EntID  eid(id, idPattern, type, typePattern);

    if (std::find(eivP->begin(), eivP->end(), eid) == eivP->end())  // if not already included
    {
      eivP->push_back(eid);
    }
  }

  return true;
}
