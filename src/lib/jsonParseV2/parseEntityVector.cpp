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

#include "logMsg/logMsg.h"

#include "ngsi/ContextAttribute.h"

#include "apiTypesV2/Entities.h"
#include "rest/ConnectionInfo.h"
#include "jsonParseV2/jsonParseTypeNames.h"
#include "jsonParseV2/parseEntityObject.h"
#include "jsonParseV2/parseEntityVector.h"



/* ****************************************************************************
*
* parseEntityVector -
*/
std::string parseEntityVector
(
  ConnectionInfo*                               ciP,
  const rapidjson::Value::ConstMemberIterator&  iter,
  Entities*                                     evP,
  bool                                          idPatternAllowed,
  bool                                          attributesAllowed
)
{
  std::string type = jsonParseTypeNames[iter->value.GetType()];

  if (type != "Array")
  {
    return "not a JSON array";
  }

  for (rapidjson::Value::ConstValueIterator iter2 = iter->value.Begin(); iter2 != iter->value.End(); ++iter2)
  {
    std::string  r;
    Entity*      eP = new Entity();

    evP->vec.push_back(eP);

    //
    // FIXME P5: is the logic of this function correct?
    // I mean, parseEntityObject() could result in error, so the push_back maybe
    // should be done after the parseEntityObject() call (and delete the eP just before return r).
    // Or maybe it doesn't matter... as in the case of error, the error is propagated back to result in
    // an OrionError and the evP is not actually used (and properly destroyed during
    // the release step)
    //
    r = parseEntityObject(ciP, iter2, eP, idPatternAllowed, attributesAllowed);
    if (r != "OK")
    {
      return r;
    }
  }

  return "OK";
}
