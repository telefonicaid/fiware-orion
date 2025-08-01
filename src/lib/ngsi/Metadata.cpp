/*
*
* Copyright 2013 Telefonica Investigacion y Desarrollo, S.A.U
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
#include <stdio.h>
#include <string>

#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"

#include "common/globals.h"
#include "common/limits.h"
#include "common/string.h"
#include "common/JsonHelper.h"
#include "alarmMgr/alarmMgr.h"

#include "orionTypes/OrionValueType.h"
#include "parse/forbiddenChars.h"
#include "ngsi/Metadata.h"

#include "mongoBackend/dbConstants.h"
#include "mongoBackend/compoundResponses.h"
#include "mongoBackend/compoundValueBson.h"
#include "mongoBackend/dbFieldEncoding.h"

#include "mongoDriver/safeMongo.h"



/* ****************************************************************************
*
* Metadata::~Metadata -
*/
Metadata::~Metadata()
{
  release();
}



/* ****************************************************************************
*
* Metadata::Metadata -
*/
Metadata::Metadata()
{
  name            = "";
  type            = "";
  stringValue     = "";
  valueType       = orion::ValueTypeNotGiven;
  typeGiven       = false;
  compoundValueP  = NULL;
  shadowed        = false;
}



/* ****************************************************************************
*
* Metadata::Metadata -
*/
Metadata::Metadata(Metadata* mP, bool useDefaultType)
{
  LM_T(LmtClone, ("'cloning' a Metadata"));

  name            = mP->name;
  type            = mP->type;
  valueType       = mP->valueType;
  stringValue     = mP->stringValue;
  numberValue     = mP->numberValue;
  boolValue       = mP->boolValue;
  typeGiven       = mP->typeGiven;
  compoundValueP  = (mP->compoundValueP != NULL)? mP->compoundValueP->clone() : NULL;
  shadowed        = mP->shadowed;

  if (useDefaultType && !typeGiven)
  {
    if ((compoundValueP == NULL) || (compoundValueP->valueType != orion::ValueTypeVector))
    {
      type = defaultType(valueType);
    }
    else
    {
      type = defaultType(orion::ValueTypeVector);
    }
  }
}



/* ****************************************************************************
*
* Metadata::Metadata -
*/
Metadata::Metadata(const std::string& _name, const std::string& _type, const char* _value)
{
  name            = _name;
  type            = _type;
  valueType       = orion::ValueTypeString;
  stringValue     = std::string(_value);
  typeGiven       = false;
  compoundValueP  = NULL;
  shadowed        = false;
}



/* ****************************************************************************
*
* Metadata::Metadata -
*/
Metadata::Metadata(const std::string& _name, const std::string& _type, const std::string& _value)
{
  name            = _name;
  type            = _type;
  valueType       = orion::ValueTypeString;
  stringValue     = _value;
  typeGiven       = false;
  compoundValueP  = NULL;
  shadowed        = false;
}



/* ****************************************************************************
*
* Metadata::Metadata -
*/
Metadata::Metadata(const std::string& _name, const std::string& _type, double _value)
{
  name            = _name;
  type            = _type;
  valueType       = orion::ValueTypeNumber;
  numberValue     = _value;
  typeGiven       = false;
  compoundValueP  = NULL;
  shadowed        = false;
}



/* ****************************************************************************
*
* Metadata::Metadata -
*/
Metadata::Metadata(const std::string& _name, const std::string& _type, bool _value)
{
  name            = _name;
  type            = _type;
  valueType       = orion::ValueTypeBoolean;
  boolValue       = _value;
  typeGiven       = false;
  compoundValueP  = NULL;
  shadowed        = false;
}



/* ****************************************************************************
*
* Metadata::Metadata -
*/
Metadata::Metadata(const std::string& _name, const orion::BSONObj& mdB)
{
  name            = _name;
  type            = mdB.hasField(ENT_ATTRS_MD_TYPE) ? getStringFieldF(mdB, ENT_ATTRS_MD_TYPE) : "";
  typeGiven       = (type.empty())? false : true;
  compoundValueP  = NULL;
  shadowed        = false;

  orion::BSONType bsonType = getFieldF(mdB, ENT_ATTRS_MD_VALUE).type();
  switch (bsonType)
  {
  case orion::String:
    valueType   = orion::ValueTypeString;
    stringValue = getStringFieldF(mdB, ENT_ATTRS_MD_VALUE);
    break;

  case orion::NumberDouble:
    valueType   = orion::ValueTypeNumber;
    numberValue = getNumberFieldF(mdB, ENT_ATTRS_MD_VALUE);
    break;

  case orion::Bool:
    valueType = orion::ValueTypeBoolean;
    boolValue = getBoolFieldF(mdB, ENT_ATTRS_MD_VALUE);
    break;

  case orion::jstNULL:
    valueType = orion::ValueTypeNull;
    break;

  case orion::Object:
    valueType      = orion::ValueTypeObject;
    compoundValueP = new orion::CompoundValueNode();
    compoundObjectResponse(compoundValueP, getFieldF(mdB, ENT_ATTRS_VALUE));
    compoundValueP->valueType = orion::ValueTypeObject;
    break;

  case orion::Array:
    valueType      = orion::ValueTypeVector;
    compoundValueP = new orion::CompoundValueNode();
    compoundVectorResponse(compoundValueP, getFieldF(mdB, ENT_ATTRS_VALUE));
    compoundValueP->valueType = orion::ValueTypeVector;
    break;

  default:
    valueType = orion::ValueTypeNotGiven;
    LM_E(("Runtime Error (unknown metadata value type in DB: %d, using ValueTypeNotGiven)", getFieldF(mdB, ENT_ATTRS_MD_VALUE).type()));
    break;
  }
}



/* ****************************************************************************
*
* Metadata::check -
*/
std::string Metadata::check(void)
{
  size_t len;
  char   errorMsg[128];

  if ((len = strlen(name.c_str())) < MIN_ID_LEN)
  {
    snprintf(errorMsg, sizeof errorMsg, "metadata name length: %zd, min length supported: %d", len, MIN_ID_LEN);
    alarmMgr.badInput(clientIp, errorMsg);
    return std::string(errorMsg);
  }

  if (name.empty())
  {
    alarmMgr.badInput(clientIp, "missing metadata name");
    return "missing metadata name";
  }

  if ( (len = strlen(name.c_str())) > MAX_ID_LEN)
  {
    snprintf(errorMsg, sizeof errorMsg, "metadata name length: %zd, max length supported: %d", len, MAX_ID_LEN);
    alarmMgr.badInput(clientIp, errorMsg);
    return std::string(errorMsg);
  }

  if (forbiddenIdChars(name.c_str()))
  {
    alarmMgr.badInput(clientIp, "found a forbidden character in the name of a Metadata", name);
    return "Invalid characters in metadata name";
  }

  if ( (len = strlen(type.c_str())) > MAX_ID_LEN)
  {
    snprintf(errorMsg, sizeof errorMsg, "metadata type length: %zd, max length supported: %d", len, MAX_ID_LEN);
    alarmMgr.badInput(clientIp, errorMsg);
    return std::string(errorMsg);
  }


  if ((len = strlen(type.c_str())) < MIN_ID_LEN)
  {
    snprintf(errorMsg, sizeof errorMsg, "metadata type length: %zd, min length supported: %d", len, MIN_ID_LEN);
    alarmMgr.badInput(clientIp, errorMsg);
    return std::string(errorMsg);
  }

  if (forbiddenIdChars(type.c_str()))
  {
    alarmMgr.badInput(clientIp, "found a forbidden character in the type of a Metadata", type);
    return "Invalid characters in metadata type";
  }

  if (valueType == orion::ValueTypeString)
  {
    if (forbiddenChars(stringValue.c_str()))
    {
      alarmMgr.badInput(clientIp, "found a forbidden character in the value of a Metadata", stringValue);
      return "Invalid characters in metadata value";
    }
  }

  return "OK";
}



/* ****************************************************************************
*
* release -
*/
void Metadata::release(void)
{
  if (compoundValueP != NULL)
  {
    delete compoundValueP;
    compoundValueP = NULL;
  }
}



/* ****************************************************************************
*
* toJson - 
*/
std::string Metadata::toJson(void)
{
  JsonObjectHelper jh;

  /* This is needed for entities coming from NGSIv1 (which allows empty or missing types) */
  std::string defType = defaultType(valueType);

  if (compoundValueP && compoundValueP->isVector())
  {
    defType = defaultType(orion::ValueTypeVector);
  }

  jh.addString("type", (!type.empty())? type : defType);

  if (compoundValueP != NULL)
  {
    jh.addRaw("value", compoundValueP->toJson());
  }
  else if (valueType == orion::ValueTypeString)
  {
    jh.addString("value", stringValue);
  }
  else if (valueType == orion::ValueTypeNumber)
  {
    if ((type == DATE_TYPE) || (type == DATE_TYPE_ALT))
    {
      jh.addDate("value", numberValue);
    }
    else // regular number
    {
      jh.addNumber("value", numberValue);
    }
  }
  else if (valueType == orion::ValueTypeBoolean)
  {
    jh.addBool("value", boolValue);
  }
  else if (valueType == orion::ValueTypeNull)
  {
    jh.addRaw("value", "null");
  }
  else if (valueType == orion::ValueTypeNotGiven)
  {
    LM_E(("Runtime Error (value not given for metadata %s)", name.c_str()));
  }
  else
  {
    LM_E(("Runtime Error (invalid value type %s for metadata %s)", valueTypeName(valueType), name.c_str()));
  }

  return jh.str();
}



/* ****************************************************************************
*
* Metadata::compoundItemExists - 
*/
bool Metadata::compoundItemExists(const std::string& compoundPath, orion::CompoundValueNode** compoundItemPP)
{
  std::vector<std::string>   compoundPathV;
  orion::CompoundValueNode*  current = compoundValueP;
  int                        levels;

  if (compoundPath.empty())
  {
    return false;
  }

  if (compoundValueP == NULL)
  {
    return false;
  }

  levels = stringSplit(compoundPath, '.', compoundPathV);

  if ((compoundPathV.size() == 0) || (levels == 0))
  {
    return false;
  }

  for (int ix = 0; ix < levels; ++ix)
  {
    bool found = false;

    for (unsigned int cIx = 0; cIx < current->childV.size(); ++cIx)
    {
      if (dbEncode(current->childV[cIx]->name) == compoundPathV[ix])
      {
        current = current->childV[cIx];
        found   = true;
        break;
      }
    }

    if (found == false)
    {
      return false;
    }
  }

  if (compoundItemPP != NULL)
  {
    *compoundItemPP = current;
  }

  return true;
}



/* ****************************************************************************
*
* Metadata::appendToBson -
*/
void Metadata::appendToBson(orion::BSONObjBuilder* mdBuilder, orion::BSONArrayBuilder* mdNamesBuilder)
{
  std::string type = this->type;

  if (!this->typeGiven)
  {
    if ((this->compoundValueP == NULL) || (this->compoundValueP->valueType != orion::ValueTypeVector))
    {
      type = defaultType(this->valueType);
    }
    else
    {
      type = defaultType(orion::ValueTypeVector);
    }
  }

  mdNamesBuilder->append(this->name);
  std::string effectiveName = dbEncode(this->name);

  // FIXME P8: this code probably should be refactored to be clearer and cleaner
  if (!type.empty())
  {
    orion::BSONObjBuilder bob;
    bob.append(ENT_ATTRS_MD_TYPE, type);
    switch (this->valueType)
    {
    case orion::ValueTypeString:
      bob.append(ENT_ATTRS_MD_VALUE, this->stringValue);
      mdBuilder->append(effectiveName, bob.obj());
      return;

    case orion::ValueTypeNumber:
      bob.append(ENT_ATTRS_MD_VALUE, this->numberValue);
      mdBuilder->append(effectiveName, bob.obj());
      return;

    case orion::ValueTypeBoolean:
      bob.append(ENT_ATTRS_MD_VALUE, this->boolValue);
      mdBuilder->append(effectiveName, bob.obj());
      return;

    case orion::ValueTypeNull:
      bob.appendNull(ENT_ATTRS_MD_VALUE);
      mdBuilder->append(effectiveName, bob.obj());
      return;

    case orion::ValueTypeObject:
      if (this->compoundValueP->valueType == orion::ValueTypeVector)
      {
        orion::BSONArrayBuilder ba;
        compoundValueBson(this->compoundValueP->childV, ba);
        bob.append(ENT_ATTRS_MD_VALUE, ba.arr());
        mdBuilder->append(effectiveName, bob.obj());
      }
      else
      {
        orion::BSONObjBuilder bo;
        compoundValueBson(this->compoundValueP->childV, bo);
        bob.append(ENT_ATTRS_MD_VALUE, bo.obj());
        mdBuilder->append(effectiveName, bob.obj());
      }
      break;

    default:
      LM_E(("Runtime Error (unknown metadata type: %d)", this->valueType));
    }
  }
  else
  {
    orion::BSONObjBuilder bob;
    switch (this->valueType)
    {
    case orion::ValueTypeString:
      bob.append(ENT_ATTRS_MD_VALUE, this->stringValue);
      mdBuilder->append(effectiveName, bob.obj());
      return;

    case orion::ValueTypeNumber:
      bob.append(ENT_ATTRS_MD_VALUE, this->numberValue);
      mdBuilder->append(effectiveName, bob.obj());
      return;

    case orion::ValueTypeBoolean:
      bob.append(ENT_ATTRS_MD_VALUE, this->boolValue);
      mdBuilder->append(effectiveName, bob.obj());
      return;

    case orion::ValueTypeNull:
      bob.appendNull(ENT_ATTRS_MD_VALUE);
      mdBuilder->append(effectiveName, bob.obj());
      return;

    case orion::ValueTypeObject:
      if (this->compoundValueP->isVector())
      {
        orion::BSONArrayBuilder ba;
        compoundValueBson(this->compoundValueP->childV, ba);
        bob.append(ENT_ATTRS_MD_VALUE, ba.arr());
        mdBuilder->append(effectiveName, bob.obj());
      }
      else
      {
        orion::BSONObjBuilder bo;
        bob.append(ENT_ATTRS_MD_VALUE, bo.obj());
        mdBuilder->append(effectiveName, bob.obj());
      }
      break;

    default:
      LM_E(("Runtime Error (unknown metadata type)"));
    }
  }
}


/* ****************************************************************************
*
* addToContext -
*
* FIXME P5: duplicated from ContextAttrMetibute::addToContext(). Maybe they should be unified
*/
void Metadata::addToContext(ExprContextObject* exprContextObjectP, bool legacy)
{
  if (compoundValueP != NULL)
  {
    // In legacy expression, objects are vector are strings to be stored in a std::map<std::string,std::string>
    if (valueType == orion::ValueTypeObject)
    {
      if (legacy)
      {
        exprContextObjectP->add(name, compoundValueP->toJson(), true);
      }
      else
      {
        exprContextObjectP->add(name, compoundValueP->toExprContextObject());
      }
    }
    else  // valueType == orion::ValueTypeVector
    {
      if (legacy)
      {
        exprContextObjectP->add(name, compoundValueP->toJson(), true);
      }
      else
      {
        exprContextObjectP->add(name, compoundValueP->toExprContextList());
      }
    }
  }
  else if (valueType == orion::ValueTypeNumber)
  {
    if ((type == DATE_TYPE) || (type == DATE_TYPE_ALT))
    {
      exprContextObjectP->add(name, toJsonString(isodate2str(numberValue)));
    }
    else // regular number
    {
      exprContextObjectP->add(name, numberValue);
    }
  }
  else if (valueType == orion::ValueTypeString)
  {
    exprContextObjectP->add(name, toJsonString(stringValue));
  }
  else if (valueType == orion::ValueTypeBoolean)
  {
    exprContextObjectP->add(name, boolValue);
  }
  else if (valueType == orion::ValueTypeNull)
  {
    exprContextObjectP->add(name);
  }
  else if (valueType == orion::ValueTypeNotGiven)
  {
    LM_E(("Runtime Error (value not given for metadata %s)", name.c_str()));
  }
  else
  {
    LM_E(("Runtime Error (invalid value type %s for metadata %s)", valueTypeName(valueType), name.c_str()));
  }
}