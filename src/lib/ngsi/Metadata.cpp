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
#include "ngsi/Metadata.h"



/* ****************************************************************************
*
* Metadata::Metadata -
*/
Metadata::Metadata()
{
  name         = "";
  type         = "";
  stringValue  = "";
  valueType    = MetadataValueTypeString;
}



/* ****************************************************************************
*
* Metadata::Metadata -
*
*/
Metadata::Metadata(Metadata* mP)
{
  LM_T(LmtClone, ("'cloning' a Metadata"));

  name        = mP->name;
  type        = mP->type;
  valueType   = mP->valueType;
  stringValue       = mP->stringValue;
  numberValue = mP->numberValue;
  boolValue   = mP->boolValue;

}

/* ****************************************************************************
*
* Metadata::Metadata -
*/
Metadata::Metadata(const std::string& _name, const std::string& _type, const char* _value)
{
  name      = _name;
  type      = _type;
  valueType = MetadataValueTypeString;
  stringValue     = std::string(_value);
}

/* ****************************************************************************
*
* Metadata::Metadata -
*/
Metadata::Metadata(const std::string& _name, const std::string& _type, const std::string& _value)
{
  name      = _name;
  type      = _type;
  valueType = MetadataValueTypeString;
  stringValue     = _value;
}

/* ****************************************************************************
*
* Metadata::Metadata -
*/
Metadata::Metadata(const std::string& _name, const std::string& _type, double _value)
{
  name         = _name;
  type         = _type;
  valueType    = MetadataValueTypeNumber;
  numberValue  = _value;
}

/* ****************************************************************************
*
* Metadata::Metadata -
*/
Metadata::Metadata(const std::string& _name, const std::string& _type, bool _value)
{
  name      = _name;
  type      = _type;
  valueType = MetadataValueTypeBoolean;
  boolValue = _value;
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

  if (type == "Association")
  {
    xValue = std::string("\n") + association.render(format, indent + "  ", false);
  }

  out += valueTag(indent + "  ", "value", xValue, format, false, (type == "Association"));
  out += endTag(indent, tag, format, comma);

  return out;
}



/* ****************************************************************************
*
* Metadata::check -
*/
std::string Metadata::check
(
  RequestType         requestType,
  Format              format,
  const std::string&  indent,
  const std::string&  predetectedError,
  int                 counter
)
{
  if (name == "")
  {
    return "missing metadata name";
  }

  if ((stringValue == "") && (type != "Association"))
  {
    return "missing metadata value";
  }

  if (type == "Association")
  {
     return association.check(requestType, format, indent, predetectedError, counter);
  }

  return "OK";
}



/* ****************************************************************************
*
* Metadata::present -
*/
void Metadata::present(const std::string& metadataType, int ix, const std::string& indent)
{
  LM_F(("%s%s Metadata %d:",   indent.c_str(), metadataType.c_str(), ix));
  LM_F(("%s  Name:     %s", indent.c_str(), name.c_str()));
  LM_F(("%s  Type:     %s", indent.c_str(), type.c_str()));
  LM_F(("%s  Value:    %s", indent.c_str(), stringValue.c_str()));
}



/* ****************************************************************************
*
* release -
*/
void Metadata::release(void)
{
  association.release();
}



/* ****************************************************************************
*
* fill - 
*/
void Metadata::fill(const struct Metadata& md)
{
  name        = md.name;
  type        = md.type;
  stringValue       = md.stringValue;
  association = md.association;
}

/* ****************************************************************************
*
* toStringValue -
*/
std::string Metadata::toStringValue(void)
{
  char buffer[64];

  switch (valueType)
  {
  case MetadataValueTypeString:
    return stringValue;
    break;

  case MetadataValueTypeNumber:
    snprintf(buffer, sizeof(buffer), "%f", numberValue);
    return std::string(buffer);
    break;

  case MetadataValueTypeBoolean:
    return boolValue ? "true" : "false";
    break;

  default:
    return "<unknown type>";
    break;
  }
}



/* ****************************************************************************
*
* toJson - 
*/
std::string Metadata::toJson(bool isLastElement)
{
  std::string  out;

  LM_M(("Metadata '%s' to JSON (type: '%s', valueType: %d)", name.c_str(), type.c_str(), valueType));

  if (type == "")
  {
    if (valueType == MetadataValueTypeNumber)
    {
      char num[32];
    
      snprintf(num, sizeof(num), "%f", numberValue);
      out = JSON_VALUE_NUMBER(name, num);
    }
    else if (valueType == MetadataValueTypeBoolean)
    {
      out = JSON_VALUE_BOOL(name, boolValue);
    }
    else if (valueType == MetadataValueTypeString)
    {
      out = JSON_VALUE(name, stringValue);
    }
    else
    {
      LM_E(("Bad value type for metadata %s", name.c_str()));
      out = JSON_VALUE(name, stringValue);
    }
  }
  else
  {
    out = JSON_STR(name) + ":{";
    out += JSON_VALUE("type", type) + ",";

    if (valueType == MetadataValueTypeString)
    {
      out += JSON_VALUE("value", stringValue);
    }
    else if (valueType == MetadataValueTypeNumber)
    {
      char num[32];

      snprintf(num, sizeof(num), "%f", numberValue);
      out += JSON_VALUE_NUMBER("value", num);
    }
    else if (valueType == MetadataValueTypeBoolean)
    {
      out += JSON_VALUE_BOOL("value", boolValue);
    }
    else
    {
      LM_E(("Bad value type for metadata %s", name.c_str()));
      out += JSON_VALUE("value", stringValue);
    }

    out += "}";
  }

  if (!isLastElement)
  {
    out += ",";
  }

  return out;
}
