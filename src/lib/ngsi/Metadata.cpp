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
#include "common/tag.h"
#include "alarmMgr/alarmMgr.h"

#include "orionTypes/OrionValueType.h"
#include "parse/forbiddenChars.h"
#include "ngsi/Metadata.h"

#include "mongoBackend/dbConstants.h"
#include "mongoBackend/safeMongo.h"

#include "rest/ConnectionInfo.h"

using namespace mongo;



/* ****************************************************************************
*
* Metadata::Metadata -
*/
Metadata::Metadata()
{
  name         = "";
  type         = "";
  stringValue  = "";
  valueType    = orion::ValueTypeString;
  typeGiven    = false;
}



/* ****************************************************************************
*
* Metadata::Metadata -
*
*/
Metadata::Metadata(Metadata* mP)
{
  LM_T(LmtClone, ("'cloning' a Metadata"));

  name         = mP->name;
  type         = mP->type;
  valueType    = mP->valueType;
  stringValue  = mP->stringValue;
  numberValue  = mP->numberValue;
  boolValue    = mP->boolValue;
  typeGiven    = false;
}



/* ****************************************************************************
*
* Metadata::Metadata -
*/
Metadata::Metadata(const std::string& _name, const std::string& _type, const char* _value)
{
  name         = _name;
  type         = _type;
  valueType    = orion::ValueTypeString;
  stringValue  = std::string(_value);
  typeGiven    = false;
}



/* ****************************************************************************
*
* Metadata::Metadata -
*/
Metadata::Metadata(const std::string& _name, const std::string& _type, const std::string& _value)
{
  name         = _name;
  type         = _type;
  valueType    = orion::ValueTypeString;
  stringValue  = _value;
  typeGiven    = false;
}



/* ****************************************************************************
*
* Metadata::Metadata -
*/
Metadata::Metadata(const std::string& _name, const std::string& _type, double _value)
{
  name         = _name;
  type         = _type;
  valueType    = orion::ValueTypeNumber;
  numberValue  = _value;
  typeGiven    = false;
}



/* ****************************************************************************
*
* Metadata::Metadata -
*/
Metadata::Metadata(const std::string& _name, const std::string& _type, bool _value)
{
  name         = _name;
  type         = _type;
  valueType    = orion::ValueTypeBoolean;
  boolValue    = _value;
  typeGiven    = false;
}

/* ****************************************************************************
*
* Metadata::Metadata -
*/
Metadata::Metadata(const BSONObj& mdB)
{
  name = getStringField(mdB, ENT_ATTRS_MD_NAME);
  type = mdB.hasField(ENT_ATTRS_MD_TYPE) ? getStringField(mdB, ENT_ATTRS_MD_TYPE) : "";
  switch (getField(mdB, ENT_ATTRS_MD_VALUE).type())
  {
  case String:
    valueType   = orion::ValueTypeString;
    stringValue = getStringField(mdB, ENT_ATTRS_MD_VALUE);
    break;

  case NumberDouble:
    valueType   = orion::ValueTypeNumber;
    numberValue = getField(mdB, ENT_ATTRS_MD_VALUE).Number();
    break;

  case Bool:
    valueType = orion::ValueTypeBoolean;
    boolValue = getBoolField(mdB, ENT_ATTRS_MD_VALUE);
    break;

  case jstNULL:
    valueType = orion::ValueTypeNone;
    break;

  default:
    valueType = orion::ValueTypeUnknown;
    LM_E(("Runtime Error (unknown metadata value value type in DB: %d)", getField(mdB, ENT_ATTRS_MD_VALUE).type()));
    break;
  }
}


/* ****************************************************************************
*
* Metadata::render -
*/
std::string Metadata::render(Format format, const std::string& indent, bool comma)
{
  std::string out     = "";
  std::string tag     = "contextMetadata";
  std::string xValue  = stringValue;

  out += startTag(indent, tag, tag, format, false, false);
  out += valueTag(indent + "  ", "name", name, format, true);
  out += valueTag(indent + "  ", "type", type, format, true);
  out += valueTag(indent + "  ", "value", xValue, format, false);
  out += endTag(indent, tag, format, comma);

  return out;
}



/* ****************************************************************************
*
* Metadata::check -
*/
std::string Metadata::check
(
  ConnectionInfo*     ciP,
  RequestType         requestType,
  Format              format,
  const std::string&  indent,
  const std::string&  predetectedError,
  int                 counter
)
{
  size_t len;
  char   errorMsg[128];

  if (name == "")
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

  if (forbiddenIdChars(ciP->apiVersion , name.c_str()))
  {
    alarmMgr.badInput(clientIp, "found a forbidden character in the name of a Metadata");
    return "Invalid characters in metadata name";
  }

  if ( (len = strlen(type.c_str())) > MAX_ID_LEN)
  {
    snprintf(errorMsg, sizeof errorMsg, "metadata type length: %zd, max length supported: %d", len, MAX_ID_LEN);
    alarmMgr.badInput(clientIp, errorMsg);
    return std::string(errorMsg);
  }

  if (forbiddenIdChars(ciP->apiVersion, type.c_str()))
  {
    alarmMgr.badInput(clientIp, "found a forbidden character in the type of a Metadata");
    return "Invalid characters in metadata type";
  }

  if (valueType == orion::ValueTypeString)
  {
    if (forbiddenChars(stringValue.c_str()))
    {
      alarmMgr.badInput(clientIp, "found a forbidden character in the value of a Metadata");
      return "Invalid characters in metadata value";
    }

    if (stringValue == "")
    {
      alarmMgr.badInput(clientIp, "missing metadata value");
      return "missing metadata value";
    }
  }

  return "OK";
}



/* ****************************************************************************
*
* Metadata::present -
*/
void Metadata::present(const std::string& metadataType, int ix, const std::string& indent)
{
  LM_T(LmtPresent, ("%s%s Metadata %d:",   
		    indent.c_str(), 
		    metadataType.c_str(), 
		    ix));
  LM_T(LmtPresent, ("%s  Name:     %s", 
		    indent.c_str(), 
		    name.c_str()));
  LM_T(LmtPresent, ("%s  Type:     %s", 
		    indent.c_str(), 
		    type.c_str()));
  LM_T(LmtPresent, ("%s  Value:    %s", 
		    indent.c_str(), 
		    stringValue.c_str()));
}



/* ****************************************************************************
*
* release -
*/
void Metadata::release(void)
{
}



/* ****************************************************************************
*
* fill - 
*/
void Metadata::fill(const struct Metadata& md)
{
  name         = md.name;
  type         = md.type;
  stringValue  = md.stringValue;
}

/* ****************************************************************************
*
* toStringValue -
*/
std::string Metadata::toStringValue(void) const
{
  char buffer[64];

  switch (valueType)
  {
  case orion::ValueTypeString:
    return stringValue;
    break;

  case orion::ValueTypeNumber:
    snprintf(buffer, sizeof(buffer), "%f", numberValue);
    return std::string(buffer);
    break;

  case orion::ValueTypeBoolean:
    return boolValue ? "true" : "false";
    break;

  default:
    return "<unknown type>";
    break;
  }

  // Added to avoid warning when compiling with -fstack-check -fstack-protector
  return "";
}



/* ****************************************************************************
*
* toJson - 
*/
std::string Metadata::toJson(bool isLastElement)
{
  std::string  out;

  out = JSON_STR(name) + ":{";

  out += (type != "")? JSON_VALUE("type", type) : JSON_STR("type") + ":" + "null";
  out += ",";

  if (valueType == orion::ValueTypeString)
  {
    out += JSON_VALUE("value", stringValue);
  }
  else if (valueType == orion::ValueTypeNumber)
  {
    char num[32];

    snprintf(num, sizeof(num), "%f", numberValue);
    out += JSON_VALUE_NUMBER("value", num);
  }
  else if (valueType == orion::ValueTypeBoolean)
  {
    out += JSON_VALUE_BOOL("value", boolValue);
  }
  else
  {
    LM_E(("Runtime Error (invalid value type for metadata %s)", name.c_str()));
    out += JSON_VALUE("value", stringValue);
  }

  out += "}";

  if (!isLastElement)
  {
    out += ",";
  }

  return out;
}
