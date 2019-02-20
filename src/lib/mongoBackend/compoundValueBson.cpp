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
* Author: Fermín Galán
*/
#include <string>
#include <vector>

#include "mongo/client/dbclient.h"

#include "common/string.h"
#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"

#include "parse/CompoundValueNode.h"
#include "mongoBackend/compoundValueBson.h"
#include "mongoBackend/dbFieldEncoding.h"



/* ****************************************************************************
*
* namespaces - 
*/
using mongo::BSONArrayBuilder;
using mongo::BSONObjBuilder;



/* ****************************************************************************
*
* compoundValueBson (for arrays) -
*
* strings2numbers is used only for the GEO_JSON generation logic to ensure NGSIv1
* strings are converted to numbers (strings are not allowed in GeoJSON)
*
*/
void compoundValueBson(const std::vector<orion::CompoundValueNode*>& children, BSONArrayBuilder& b, bool strings2numbers)
{
  for (unsigned int ix = 0; ix < children.size(); ++ix)
  {
    orion::CompoundValueNode* child = children[ix];

    if (child->valueType == orion::ValueTypeString)
    {
      if ((strings2numbers) && (str2double(child->stringValue.c_str(), &child->numberValue)))
      {
        b.append(child->numberValue);
      }
      else
      {
        // Fails in str2double() means that values is not an actual number, so we do nothing and leave it as it is
        b.append(child->stringValue);
      }
    }
    else if (child->valueType == orion::ValueTypeNumber)
    {
      b.append(child->numberValue);
    }
    else if (child->valueType == orion::ValueTypeBoolean)
    {
      b.append(child->boolValue);
    }
    else if (child->valueType == orion::ValueTypeNull)
    {
      b.appendNull();
    }
    else if (child->valueType == orion::ValueTypeVector)
    {
      BSONArrayBuilder ba;

      compoundValueBson(child->childV, ba, strings2numbers);
      b.append(ba.arr());
    }
    else if (child->valueType == orion::ValueTypeObject)
    {
      BSONObjBuilder bo;

      compoundValueBson(child->childV, bo, strings2numbers);
      b.append(bo.obj());
    }
    else if (child->valueType == orion::ValueTypeNotGiven)
    {
      LM_E(("Runtime Error (value not given in compound value)"));
    }
    else
    {
      LM_E(("Runtime Error (Unknown type in compound value)"));
    }
  }
}



/* ****************************************************************************
*
* compoundValueBson (for objects) -
*
* strings2numbers is used only for the GEO_JSON generation logic to ensure NGSIv1
* strings are converted to numbers (strings are not allowed in GeoJSON)
*/
void compoundValueBson(const std::vector<orion::CompoundValueNode*>& children, BSONObjBuilder& b, bool strings2numbers)
{
  for (unsigned int ix = 0; ix < children.size(); ++ix)
  {
    orion::CompoundValueNode*  child         = children[ix];
    std::string                effectiveName = dbDotEncode(child->name);

    if (child->valueType == orion::ValueTypeString)
    {
      if ((strings2numbers) && (str2double(child->stringValue.c_str(), &child->numberValue)))
      {
        b.append(effectiveName, child->numberValue);
      }
      else
      {
        // Fails in str2double() means that values is not an actual number, so we do nothing and leave it as it is
        b.append(effectiveName, child->stringValue);
      }
    }
    else if (child->valueType == orion::ValueTypeNumber)
    {
      b.append(effectiveName, child->numberValue);
    }
    else if (child->valueType == orion::ValueTypeBoolean)
    {
      b.append(effectiveName, child->boolValue);
    }
    else if (child->valueType == orion::ValueTypeNull)
    {
      b.appendNull(effectiveName);
    }
    else if (child->valueType == orion::ValueTypeVector)
    {
      BSONArrayBuilder ba;

      compoundValueBson(child->childV, ba, strings2numbers);
      b.append(effectiveName, ba.arr());
    }
    else if (child->valueType == orion::ValueTypeObject)
    {
      BSONObjBuilder bo;

      compoundValueBson(child->childV, bo, strings2numbers);
      b.append(effectiveName, bo.obj());
    }
    else if (child->valueType == orion::ValueTypeNotGiven)
    {
      LM_E(("Runtime Error (value not given in compound value)"));
    }
    else
    {
      LM_E(("Runtime Error (Unknown type in compound value)"));
    }
  }
}
