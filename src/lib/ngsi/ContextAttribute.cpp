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

#include "common/string.h"

#include "common/globals.h"
#include "common/tag.h"
#include "common/limits.h"
#include "alarmMgr/alarmMgr.h"
#include "orionTypes/OrionValueType.h"
#include "parse/forbiddenChars.h"
#include "apiTypesV2/ErrorCode.h"
#include "ngsi/ContextAttribute.h"
#include "rest/ConnectionInfo.h"
#include "rest/uriParamNames.h"

using namespace orion;



/* ****************************************************************************
*
* ContextAttribute::~ContextAttribute - 
*/
ContextAttribute::~ContextAttribute()
{
}



/* ****************************************************************************
*
* ContextAttribute::ContextAttribute - 
*/
ContextAttribute::ContextAttribute()
{
  LM_T(LmtClone, ("Creating a ContextAttribute 1"));
  name                  = "";
  type                  = "";
  stringValue           = "";
  valueType             = orion::ValueTypeString;
  compoundValueP        = NULL;
  typeFromXmlAttribute  = "";
  found                 = false;
  skip                  = false;
  typeGiven             = false;

  providingApplication.set("");
  providingApplication.setFormat(NOFORMAT);
}



/* ****************************************************************************
*
* ContextAttribute::ContextAttribute - 
*
* Note that this constructor moves the compoundValue of the source CA to the
* CA being constructed (the compoundValueP attribute in the source CA is set to NULL).
* Another option (closer to copy semantics) would be cloning (using the clone() method in
* CompoundValueNode class) but by the moment this is not needed by this constructor as
* all their usage cases suffice with this "move compoundValue instead of cloning" approach.
*
* Note however that the treatement of metadata is different: in that case, the metadata
* in "cloned" from source CA to the CA being constructed.
*
*/
ContextAttribute::ContextAttribute(ContextAttribute* caP, bool useDefaultType)
{
  name                  = caP->name;
  type                  = caP->type;
  valueType             = caP->valueType;
  stringValue           = caP->stringValue;
  numberValue           = caP->numberValue;
  boolValue             = caP->boolValue;
  compoundValueP        = caP->compoundValueP;
  caP->compoundValueP   = NULL;
  found                 = caP->found;
  typeFromXmlAttribute  = "";
  skip                  = false;
  typeGiven             = caP->typeGiven;

  providingApplication.set(caP->providingApplication.get());
  providingApplication.setFormat(caP->providingApplication.getFormat());

  LM_T(LmtClone, ("Creating a ContextAttribute: compoundValueP at %p for attribute '%s' at %p",
                  compoundValueP,
                  name.c_str(),
                  this));

  // Cloning metadata
  for (unsigned int mIx = 0; mIx < caP->metadataVector.size(); ++mIx)
  {
    LM_T(LmtClone, ("Copying metadata %d", mIx));
    Metadata* mP = new Metadata(caP->metadataVector[mIx], useDefaultType);
    metadataVector.push_back(mP);
  }

  if (useDefaultType && (type == ""))
  {
    type = DEFAULT_TYPE;
  }
}



/* ****************************************************************************
*
* ContextAttribute::ContextAttribute -
*/
ContextAttribute::ContextAttribute
(
  const std::string&  _name,
  const std::string&  _type,
  const char*         _value,
  bool                _found
)
{
  LM_T(LmtClone, ("Creating a string ContextAttribute '%s':'%s':'%s', setting its compound to NULL",
                  _name.c_str(),
                  _type.c_str(),
                  _value));

  name                  = _name;
  type                  = _type;
  stringValue           = std::string(_value);
  valueType             = orion::ValueTypeString;
  compoundValueP        = NULL;
  found                 = _found;
  skip                  = false;
  typeGiven             = false;

  providingApplication.set("");
  providingApplication.setFormat(NOFORMAT);
}



/* ****************************************************************************
*
* ContextAttribute::ContextAttribute - 
*/
ContextAttribute::ContextAttribute
(
  const std::string&  _name,
  const std::string&  _type,
  const std::string&  _value,
  bool                _found
)
{
  LM_T(LmtClone, ("Creating a string ContextAttribute '%s':'%s':'%s', setting its compound to NULL",
                  _name.c_str(),
                  _type.c_str(),
                  _value.c_str()));

  name                  = _name;
  type                  = _type;
  stringValue           = _value;
  valueType             = orion::ValueTypeString;
  compoundValueP        = NULL;
  found                 = _found;
  skip                  = false;
  typeGiven             = false;

  providingApplication.set("");
  providingApplication.setFormat(NOFORMAT);
}



/* ****************************************************************************
*
* ContextAttribute::ContextAttribute -
*/
ContextAttribute::ContextAttribute
(
  const std::string&  _name,
  const std::string&  _type,
  double              _value,
  bool                _found
)
{
  LM_T(LmtClone, ("Creating a number ContextAttribute '%s':'%s':'%d', setting its compound to NULL",
                  _name.c_str(),
                  _type.c_str(),
                  _value));

  name                  = _name;
  type                  = _type;
  numberValue           = _value;
  valueType             = orion::ValueTypeNumber;
  compoundValueP        = NULL;
  found                 = _found;
  skip                  = false;
  typeGiven             = false;

  providingApplication.set("");
  providingApplication.setFormat(NOFORMAT);
}



/* ****************************************************************************
*
* ContextAttribute::ContextAttribute -
*/
ContextAttribute::ContextAttribute
(
  const std::string&  _name,
  const std::string&  _type,
  bool                _value,
  bool                _found
)
{
  LM_T(LmtClone, ("Creating a boolean ContextAttribute '%s':'%s':'%s', setting its compound to NULL",
                  _name.c_str(),
                  _type.c_str(),
                  _value ? "true" : "false"));

  name                  = _name;
  type                  = _type;
  boolValue             = _value;
  valueType             = orion::ValueTypeBoolean;
  compoundValueP        = NULL;
  found                 = _found;
  skip                  = false;
  typeGiven             = false;

  providingApplication.set("");
  providingApplication.setFormat(NOFORMAT);
}




/* ****************************************************************************
*
* ContextAttribute::ContextAttribute -
*/
ContextAttribute::ContextAttribute
(
  const std::string&         _name,
  const std::string&         _type,
  orion::CompoundValueNode*  _compoundValueP
)
{
  LM_T(LmtClone, ("Creating a ContextAttribute, maintaining a pointer to compound value (at %p)", _compoundValueP));

  name                  = _name;
  type                  = _type;
  compoundValueP        = _compoundValueP->clone();
  typeFromXmlAttribute  = "";
  found                 = false;
  valueType             = orion::ValueTypeObject;  // FIXME P6: Could be ValueTypeVector ...
  skip                  = false;
  typeGiven             = false;

  providingApplication.set("");
  providingApplication.setFormat(NOFORMAT);
}



/* ****************************************************************************
*
* ContextAttribute::getId() -
*/
std::string ContextAttribute::getId(void) const
{
  for (unsigned int ix = 0; ix < metadataVector.size(); ++ix)
  {
    if (metadataVector[ix]->name == NGSI_MD_ID)
    {
      return metadataVector[ix]->stringValue;
    }
  }

  return "";
}



/* ****************************************************************************
*
* ContextAttribute::getLocation() -
*/
std::string ContextAttribute::getLocation(const std::string& apiVersion) const
{
  if (apiVersion == "v1")
  {
    for (unsigned int ix = 0; ix < metadataVector.size(); ++ix)
    {
      if (metadataVector[ix]->name == NGSI_MD_LOCATION)
      {
        return metadataVector[ix]->stringValue;
      }
    }
  }
  else // v2
  {
    if ((type == GEO_POINT) || (type == GEO_LINE) || (type == GEO_BOX) || (type == GEO_POLYGON))
    {
      return LOCATION_WGS84;
    }
  }

  return "";
}



/* ****************************************************************************
*
* renderAsJsonObject - 
*/
std::string ContextAttribute::renderAsJsonObject
(
  ConnectionInfo*     ciP,
  RequestType         request,
  const std::string&  indent,
  bool                comma,
  bool                omitValue
)
{
  std::string  out                    = "";
  std::string  jsonTag                = name;
  bool         commaAfterContextValue = metadataVector.size() != 0;
  bool         commaAfterType         = !omitValue || commaAfterContextValue;

  out += startTag(indent, "", jsonTag, ciP->outFormat, false, true);
  out += valueTag(indent + "  ", "type",         type,  ciP->outFormat, commaAfterType);

  if (compoundValueP == NULL)
  {
    if (omitValue == false)
    {
      std::string effectiveValue        = "";
      bool        valueIsNumberOrBool   = false;

      switch (valueType)
      {
      case ValueTypeString:
        effectiveValue = stringValue;
        break;

      case ValueTypeBoolean:
        effectiveValue      = boolValue? "true" : "false";
        valueIsNumberOrBool = true;
        break;

      case ValueTypeNumber:
        if (type == DATE_TYPE)
        {
          effectiveValue = isodate2str(numberValue);
        }
        else // regular number
        {
          effectiveValue      = toString(numberValue);
          valueIsNumberOrBool = true;
        }
        break;

      case ValueTypeNone:
        effectiveValue = "null";
        break;

      default:
        LM_E(("Runtime Error (unknown value type: %d)", valueType));
      }

      //
      // NOTE
      // renderAsJsonObject is used in v1 only.
      // => we only need to care about stringValue (not boolValue nor numberValue)
      //
      out += valueTag(indent + "  ", ((ciP->outFormat == XML)? "contextValue" : "value"),
                      (request != RtUpdateContextResponse)? effectiveValue : "",
                      ciP->outFormat, commaAfterContextValue, valueIsNumberOrBool);
    }
  }
  else
  {
    bool isCompoundVector = false;

    if ((compoundValueP != NULL) && (compoundValueP->valueType == orion::ValueTypeVector))
    {
      isCompoundVector = true;
    }

    out += startTag(indent + "  ", "contextValue", "value", ciP->outFormat, isCompoundVector, true, isCompoundVector);
    out += compoundValueP->render(ciP, ciP->outFormat, indent + "    ");
    out += endTag(indent + "  ", "contextValue", ciP->outFormat, commaAfterContextValue, isCompoundVector);
  }

  if (omitValue == false)
  {
    out += metadataVector.render(ciP->outFormat, indent + "  ", false);
  }

  out += endTag(indent, "", ciP->outFormat, comma);

  return out;
}

/* ****************************************************************************
*
* renderAsNameString -
*/
std::string ContextAttribute::renderAsNameString
(
  ConnectionInfo*     ciP,
  RequestType         request,
  const std::string&  indent,
  bool                comma
)
{
  std::string  out                    = "";

  if (ciP->outFormat == XML)
  {
    out += indent + "<name>" + name + "</name>\n";
  }
  else /* JSON */
  {
    if (comma)
    {
      out += indent + "\"" + name + "\",\n";
    }
    else
    {
      out += indent + "\"" + name + "\"\n";
    }
  }

  return out;

}

/* ****************************************************************************
*
* render - 
*/
std::string ContextAttribute::render
(
  ConnectionInfo*     ciP,
  RequestType         request,
  const std::string&  indent,
  bool                comma,
  bool                omitValue
)
{
  std::string  out                    = "";
  std::string  xmlTag                 = "contextAttribute";
  std::string  jsonTag                = "attribute";
  bool         valueRendered          = (compoundValueP != NULL) || (omitValue == false) || (request == RtUpdateContextResponse);
  bool         commaAfterContextValue = metadataVector.size() != 0;
  bool         commaAfterType         = valueRendered;

  metadataVector.tagSet("metadata");

  if ((ciP->uriParam[URI_PARAM_ATTRIBUTE_FORMAT] == "object") && (ciP->outFormat == JSON))
  {
    return renderAsJsonObject(ciP, request, indent, comma, omitValue);
  }

  out += startTag(indent, xmlTag, jsonTag, ciP->outFormat, false, false);
  out += valueTag(indent + "  ", "name",         name,  ciP->outFormat, true);  // attribute.type is always rendered
  out += valueTag(indent + "  ", "type",         type,  ciP->outFormat, commaAfterType);

  if (compoundValueP == NULL)
  {
    if (omitValue == false)
    {
      std::string effectiveValue      = "";
      bool        valueIsNumberOrBool = false;

      switch (valueType)
      {
      case ValueTypeString:
        effectiveValue = stringValue;
        break;

      case ValueTypeBoolean:
        effectiveValue      = boolValue? "true" : "false";
        valueIsNumberOrBool = true;
        break;

      case ValueTypeNumber:
        if (type == DATE_TYPE)
        {
          effectiveValue = isodate2str(numberValue);
        }
        else // regular number
        {
          effectiveValue      = toString(numberValue);
          valueIsNumberOrBool = true;
        }
        break;

      case ValueTypeNone:
        effectiveValue = "null";
        break;

      default:
        LM_E(("Runtime Error (unknown value type: %d)", valueType));
      }

      out += valueTag(indent + "  ", ((ciP->outFormat == XML)? "contextValue" : "value"),
                        "value",
                        (request != RtUpdateContextResponse)? effectiveValue : "",
                        ciP->outFormat, commaAfterContextValue, valueIsNumberOrBool);

    }
    else if (request == RtUpdateContextResponse)
    {
      out += valueTag(indent + "  ", ((ciP->outFormat == XML)? "contextValue" : "value"),
                      "", ciP->outFormat, commaAfterContextValue);
    }
  }
  else
  {
    bool isCompoundVector = false;

    if ((compoundValueP != NULL) && (compoundValueP->valueType == orion::ValueTypeVector))
    {
      isCompoundVector = true;
    }

    out += startTag(indent + "  ", "contextValue", "value", ciP->outFormat, isCompoundVector, true, isCompoundVector);
    out += compoundValueP->render(ciP, ciP->outFormat, indent + "    ");
    out += endTag(indent + "  ", "contextValue", ciP->outFormat, commaAfterContextValue, isCompoundVector);
  }

  out += metadataVector.render(ciP->outFormat, indent + "  ", false);
  out += endTag(indent, xmlTag, ciP->outFormat, comma);

  return out;
}



/* ****************************************************************************
*
* toJson -
*
* FIXME: Refactor this method in order to simplify
*        the code paths of the rendering process
*
*/
std::string ContextAttribute::toJson(bool isLastElement, bool types, const std::string& renderMode, RequestType requestType)
{
  std::string  out;

  if (types == true)
  {
    out = JSON_STR(name) + ":{" + JSON_STR("type") + ":" + JSON_STR(type) + "}"; 
  }
  else if ((renderMode == "values") || (renderMode == "keyValues"))
  {
    out = (renderMode == "keyValues")? JSON_STR(name) + ":" : "";

    if (compoundValueP != NULL)
    {
      if (compoundValueP->isObject())
      {
        out += "{" + compoundValueP->toJson(true) + "}";
      }
      else if (compoundValueP->isVector())
      {
        out += "[" + compoundValueP->toJson(true) + "]";
      }
    }
    else if (valueType == orion::ValueTypeNumber)
    {
      if (type == DATE_TYPE)
      {
        out += JSON_STR(isodate2str(numberValue));
      }
      else // regular number
      {
        out += toString(numberValue);
      }
    }
    else if (valueType == orion::ValueTypeString)
    {
      out += JSON_STR(stringValue);
    }
    else if (valueType == orion::ValueTypeBoolean)
    {
      out += (boolValue == true)? "true" : "false";
    }
    else if (valueType == orion::ValueTypeNone)
    {
      out += "null";
    }
  }
  else  // Render mode: normalized 
  {
    if (requestType != EntityAttributeResponse)
    {
      out = JSON_STR(name) + ":{";
    }

    //
    // type
    //
    /* This is needed for entities coming from NGSIv1 (which allows empty or missing types) */
    out += (type != "")? JSON_VALUE("type", type) : JSON_VALUE("type", DEFAULT_TYPE);
    out += ",";


    //
    // value
    //
    if (compoundValueP != NULL)
    {
      if (compoundValueP->isObject())
      {
        out += JSON_STR("value") + ":{" + compoundValueP->toJson(true) + "}";
      }
      else if (compoundValueP->isVector())
      {
        out += JSON_STR("value") + ":[" + compoundValueP->toJson(true) + "]";
      }
    }
    else if (valueType == orion::ValueTypeNumber)
    {
      if (type == DATE_TYPE)
      {
        out += JSON_VALUE("value", isodate2str(numberValue));;
      }
      else // regular number
      {
        out += JSON_VALUE_NUMBER("value", toString(numberValue));
      }
    }
    else if (valueType == orion::ValueTypeString)
    {
      out += JSON_VALUE("value", stringValue);
    }
    else if (valueType == orion::ValueTypeBoolean)
    {
      out += JSON_VALUE_BOOL("value", boolValue);
    }
    else if (valueType == orion::ValueTypeNone)
    {
      out += JSON_STR("value") + ":" + "null";
    }
    else
    {
      out += JSON_VALUE("value", stringValue);
    }
    out += ",";

    //
    // metadata
    //
    out += JSON_STR("metadata") + ":" + "{" + metadataVector.toJson(true) + "}";

    if (requestType != EntityAttributeResponse)
    {
      out += "}";
    }
  }

  if (!isLastElement)
  {
    out += ",";
  }

  return out;
}



/* ****************************************************************************
*
* toJsonAsValue -
*/
std::string ContextAttribute::toJsonAsValue(ConnectionInfo* ciP)
{
  std::string  out;

  if (ciP->outFormat == JSON)
  {
    if (compoundValueP != NULL)
    {
      if (compoundValueP->isVector())
      {
        out = "[" + compoundValueP->toJson(true) + "]";
      }
      else  // Object
      {
        out = "{" + compoundValueP->toJson(false) + "}";
      }
    }
    else
    {
      ErrorCode ec("NotAcceptable", "accepted MIME types: text/plain");
      ciP->httpStatusCode = SccNotAcceptable;

      out = ec.toJson(true);
    }
  }
  else  // TEXT
  {
    if (compoundValueP != NULL)
    {
      if (compoundValueP->isVector())
      {
        out = "[" + compoundValueP->toJson(false) + "]";
      }
      else  // Object
      {
        out = "{" + compoundValueP->toJson(false) + "}";
      }
    }
    else
    {
      char buf[64];

      switch (valueType)
      {
      case orion::ValueTypeString:
        out = stringValue;
        break;

      case orion::ValueTypeNumber:
        if (type == DATE_TYPE)
        {
          out = isodate2str(numberValue);
        }
        else // regular number
        {
          snprintf(buf, sizeof(buf), "%f", numberValue);
          out = buf;
        }
        break;

      case orion::ValueTypeBoolean:
        snprintf(buf, sizeof(buf), "%s", boolValue? "true" : "false");
        out = buf;
        break;

      default:
        out = "ERROR";
        break;
      }
    }
  }

  return out;
}



/* ****************************************************************************
*
* ContextAttribute::check - 
*/
std::string ContextAttribute::check
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
  char errorMsg[128];

  if ((name == "") && (requestType != EntityAttributeValueRequest))
  {
    return "missing attribute name";
  }

  if ( (len = strlen(name.c_str())) > MAX_ID_LEN)
  {
    snprintf(errorMsg, sizeof errorMsg, "attribute name length: %zd, max length supported: %d", len, MAX_ID_LEN);
    alarmMgr.badInput(clientIp, errorMsg);
    return std::string(errorMsg);
  }

  if (forbiddenIdChars(ciP->apiVersion, name.c_str()))
  {
    alarmMgr.badInput(clientIp, "found a forbidden character in the name of an attribute");
    return "Invalid characters in attribute name";
  }

  if ( (len = strlen(type.c_str())) > MAX_ID_LEN)
  {
    snprintf(errorMsg, sizeof errorMsg, "attribute type length: %zd, max length supported: %d", len, MAX_ID_LEN);
    alarmMgr.badInput(clientIp, errorMsg);
    return std::string(errorMsg);
  }

  if (forbiddenIdChars(ciP->apiVersion, type.c_str()))
  {
    alarmMgr.badInput(clientIp, "found a forbidden character in the type of an attribute");
    return "Invalid characters in attribute type";
  }

  if ((compoundValueP != NULL) && (compoundValueP->childV.size() != 0))
  {
    // FIXME P9: Use CompoundValueNode::check here and stop calling it from where it is called right now.
    //           Also, change CompoundValueNode::check to return std::string
    return "OK";
  }

  if (valueType == orion::ValueTypeString)
  {
    if (forbiddenChars(stringValue.c_str()))
    {
      alarmMgr.badInput(clientIp, "found a forbidden character in the value of an attribute");
      return "Invalid characters in attribute value";
    }
  }

  return metadataVector.check(ciP, requestType, format, indent + "  ", predetectedError, counter);
}



/* ****************************************************************************
*
* ContextAttribute::present - 
*/
void ContextAttribute::present(const std::string& indent, int ix)
{
  LM_T(LmtPresent, ("%sAttribute %d:",    
		    indent.c_str(), 
		    ix));
  LM_T(LmtPresent, ("%s  Name:      %s", 
		    indent.c_str(), 
		    name.c_str()));
  LM_T(LmtPresent, ("%s  Type:      %s", 
		    indent.c_str(), 
		    type.c_str()));

  if (compoundValueP == NULL)
  {
    if (valueType == orion::ValueTypeString)
    {
      LM_T(LmtPresent, ("%s  String Value:      %s", 
			indent.c_str(), 
			stringValue.c_str()));
    }
    else if (valueType == orion::ValueTypeNumber)
    {
      LM_T(LmtPresent, ("%s  Number Value:      %f", 
			indent.c_str(), 
			numberValue));
    }
    else if (valueType == orion::ValueTypeBoolean)
    {
      LM_T(LmtPresent, ("%s  Boolean Value:      %s", 
			indent.c_str(), 
			(boolValue == false)? "false" : "true"));
    }
    else if (valueType == orion::ValueTypeNone)
    {
      LM_T(LmtPresent, ("%s  No Value", indent.c_str()));
    }
    else
    {
      LM_T(LmtPresent, ("%s  Unknown value type (%d)", 
			indent.c_str(), 
			valueType));
    }
  }
  else
  {
    compoundValueP->show(indent + "  ");
  }

  LM_T(LmtPresent, ("%s  PA:       %s (%s)", 
		    indent.c_str(), 
		    providingApplication.get().c_str(), 
		    formatToString(providingApplication.getFormat())));
  LM_T(LmtPresent, ("%s  found:    %s", 
		    indent.c_str(), 
		    FT(found)));
  LM_T(LmtPresent, ("%s  skip:     %s", 
		    indent.c_str(), 
		    FT(skip)));

  metadataVector.present("Attribute", indent + "  ");
}



/* ****************************************************************************
*
* ContextAttribute::release - 
*/
void ContextAttribute::release(void)
{
  if (compoundValueP != NULL)
  {
    delete compoundValueP;
    compoundValueP = NULL;
  }

  metadataVector.release();
}



/* ****************************************************************************
*
* ContextAttribute::getName -
*/
std::string ContextAttribute::getName(void)
{
  return name;
}

/* ****************************************************************************
*
* ContextAttribute::getValue -
*/
std::string ContextAttribute::getValue(void) const
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
* clone - 
*/
ContextAttribute* ContextAttribute::clone(void)
{
  return new ContextAttribute(this);
}
