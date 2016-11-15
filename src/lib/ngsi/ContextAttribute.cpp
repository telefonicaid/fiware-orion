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
#include "common/RenderFormat.h"
#include "alarmMgr/alarmMgr.h"
#include "orionTypes/OrionValueType.h"
#include "parse/forbiddenChars.h"
#include "ngsi/ContextAttribute.h"
#include "rest/ConnectionInfo.h"
#include "rest/uriParamNames.h"
#include "rest/OrionError.h"
#include "parse/CompoundValueNode.h"

#include "mongo/client/dbclient.h"
#include "mongoBackend/dbConstants.h"
#include "mongoBackend/dbFieldEncoding.h"
#include "mongoBackend/compoundValueBson.h"

using namespace mongo;
using namespace orion;



/* ****************************************************************************
*
* ContextAttribute::bsonAppendAttrValue -
*
*/
void ContextAttribute::bsonAppendAttrValue(BSONObjBuilder& bsonAttr) const
{
  switch (valueType)
  {
    case ValueTypeString:
      bsonAttr.append(ENT_ATTRS_VALUE, stringValue);
      break;

    case ValueTypeNumber:
      bsonAttr.append(ENT_ATTRS_VALUE, numberValue);
      break;

    case ValueTypeBoolean:
      bsonAttr.append(ENT_ATTRS_VALUE, boolValue);
      break;

    case ValueTypeNone:
      bsonAttr.appendNull(ENT_ATTRS_VALUE);
      break;

    default:
      LM_E(("Runtime Error (unknown attribute type: %d)", valueType));
  }
}


/* ****************************************************************************
*
* ContextAttribute::valueBson -
*
* Used to render attribute value to BSON, appended into the bsonAttr builder
*/
void ContextAttribute::valueBson(BSONObjBuilder& bsonAttr) const
{
  if (compoundValueP == NULL)
  {
    bsonAppendAttrValue(bsonAttr);
  }
  else
  {
    if (compoundValueP->valueType == ValueTypeVector)
    {
      BSONArrayBuilder b;
      compoundValueBson(compoundValueP->childV, b);
      bsonAttr.append(ENT_ATTRS_VALUE, b.arr());
    }
    else if (compoundValueP->valueType == ValueTypeObject)
    {
      BSONObjBuilder b;

      compoundValueBson(compoundValueP->childV, b);
      bsonAttr.append(ENT_ATTRS_VALUE, b.obj());
    }
    else if (compoundValueP->valueType == ValueTypeString)
    {
      // FIXME P4: this is somehow redundant. See https://github.com/telefonicaid/fiware-orion/issues/271
      bsonAttr.append(ENT_ATTRS_VALUE, compoundValueP->stringValue);
    }
    else if (compoundValueP->valueType == ValueTypeNumber)
    {
      // FIXME P4: this is somehow redundant. See https://github.com/telefonicaid/fiware-orion/issues/271
      bsonAttr.append(ENT_ATTRS_VALUE, compoundValueP->numberValue);
    }
    else if (compoundValueP->valueType == ValueTypeBoolean)
    {
      // FIXME P4: this is somehow redundant. See https://github.com/telefonicaid/fiware-orion/issues/271
      bsonAttr.append(ENT_ATTRS_VALUE, compoundValueP->boolValue);
    }
    else if (compoundValueP->valueType == ValueTypeNone)
    {
      // FIXME P4: this is somehow redundant. See https://github.com/telefonicaid/fiware-orion/issues/271
      bsonAttr.appendNull(ENT_ATTRS_VALUE);
    }
    else
    {
      LM_E(("Runtime Error (Unknown type in compound value)"));
    }
  }
}



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
  found                 = false;
  skip                  = false;
  typeGiven             = false;
  onlyValue             = false;
  previousValue         = NULL;

  creDate = 0;
  modDate = 0;

  providingApplication.set("");
  providingApplication.setMimeType(NOMIMETYPE);
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
  skip                  = false;
  typeGiven             = caP->typeGiven;
  onlyValue             = caP->onlyValue;
  previousValue         = NULL;

  creDate = caP->creDate;
  modDate = caP->modDate;

  providingApplication.set(caP->providingApplication.get());
  providingApplication.setMimeType(caP->providingApplication.getMimeType());

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

  if (useDefaultType && !typeGiven)
  {
    //
    // NOTE:
    //   Compound values all have attribute type == orion::ValueTypeObject.
    //   So, if compound, we need to step down into the compound to see whether it is 
    //   an object or a vector.
    //
    if ((valueType != orion::ValueTypeObject) || (compoundValueP == NULL) || (compoundValueP->valueType != orion::ValueTypeVector))
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
  onlyValue             = false;
  previousValue         = NULL;

  creDate = 0;
  modDate = 0;

  providingApplication.set("");
  providingApplication.setMimeType(NOMIMETYPE);
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
  onlyValue             = false;
  previousValue         = NULL;

  creDate = 0;
  modDate = 0;

  providingApplication.set("");
  providingApplication.setMimeType(NOMIMETYPE);
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
  onlyValue             = false;
  previousValue         = NULL;

  creDate = 0;
  modDate = 0;

  providingApplication.set("");
  providingApplication.setMimeType(NOMIMETYPE);
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
  previousValue         = NULL;

  creDate = 0;
  modDate = 0;

  providingApplication.set("");
  providingApplication.setMimeType(NOMIMETYPE);
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
  found                 = false;
  valueType             = orion::ValueTypeObject;  // FIXME P6: Could be ValueTypeVector ...
  skip                  = false;
  typeGiven             = false;
  previousValue         = NULL;

  creDate = 0;
  modDate = 0;

  providingApplication.set("");
  providingApplication.setMimeType(NOMIMETYPE);
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
    // Deprecated way, but still supported
    for (unsigned int ix = 0; ix < metadataVector.size(); ++ix)
    {
      if (metadataVector[ix]->name == NGSI_MD_LOCATION)
      {
        return metadataVector[ix]->stringValue;
      }
    }

    // Current way of declaring location in NGSIv1, aligned with NGSIv2 (note that not all NGSIv1 geo:xxxx
    // are supported, only geo:point)
    if (type == GEO_POINT)
    {
      return LOCATION_WGS84;
    }
  }
  else // v2
  {
    if ((type == GEO_POINT) || (type == GEO_LINE) || (type == GEO_BOX) || (type == GEO_POLYGON) || (type == GEO_JSON))
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
  std::string  key                    = name;
  bool         commaAfterContextValue = metadataVector.size() != 0;
  bool         commaAfterType         = !omitValue || commaAfterContextValue;

  out += startTag2(indent, key, false, true);
  out += valueTag1(indent + "  ", "type",         type,  commaAfterType);

  if (compoundValueP == NULL)
  {
    if (omitValue == false)
    {
      std::string effectiveValue  = "";
      bool        withoutQuotes   = false;

      switch (valueType)
      {
      case ValueTypeString:
        effectiveValue = stringValue;
        break;

      case ValueTypeBoolean:
        effectiveValue = boolValue? "true" : "false";
        withoutQuotes  = true;
        break;

      case ValueTypeNumber:
        if (type == DATE_TYPE)
        {
          effectiveValue = isodate2str(numberValue);
        }
        else // regular number
        {
          effectiveValue = toString(numberValue);
          withoutQuotes  = true;
        }
        break;

      case ValueTypeNone:
        effectiveValue = "null";
        withoutQuotes  = true;
        break;

      default:
        LM_E(("Runtime Error (unknown value type: %d)", valueType));
      }

      //
      // NOTE
      // renderAsJsonObject is used in v1 only.
      // => we only need to care about stringValue (not boolValue, numberValue nor nullValue)
      //
      out += valueTag1(indent + "  ", "value",
                            (request != RtUpdateContextResponse)? effectiveValue : "",
                            commaAfterContextValue, false, withoutQuotes);
    }
  }
  else
  {
    bool isCompoundVector = false;

    if ((compoundValueP != NULL) && (compoundValueP->valueType == orion::ValueTypeVector))
    {
      isCompoundVector = true;
    }

    out += startTag2(indent + "  ", "value", isCompoundVector, true);
    out += compoundValueP->render(ciP, indent + "    ");
    out += endTag(indent + "  ", commaAfterContextValue, isCompoundVector);
  }

  if (omitValue == false)
  {
    out += metadataVector.render(indent + "  ", false);
  }

  out += endTag(indent, comma);

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

  if (comma)
  {
    out += indent + "\"" + name + "\",\n";
  }
  else
  {
    out += indent + "\"" + name + "\"\n";
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
  std::string  key                    = "attribute";
  bool         valueRendered          = (compoundValueP != NULL) || (omitValue == false) || (request == RtUpdateContextResponse);
  bool         commaAfterContextValue = metadataVector.size() != 0;
  bool         commaAfterType         = valueRendered;

  metadataVector.keyNameSet("metadata");

  if ((ciP->uriParam[URI_PARAM_ATTRIBUTE_FORMAT] == "object") && (ciP->outMimeType == JSON))
  {
    return renderAsJsonObject(ciP, request, indent, comma, omitValue);
  }

  out += startTag2(indent, key, false, false);
  out += valueTag1(indent + "  ", "name", name,  true);  // attribute.type is always rendered
  out += valueTag1(indent + "  ", "type", type,  commaAfterType);

  if (compoundValueP == NULL)
  {
    if (omitValue == false)
    {
      std::string effectiveValue = "";
      bool        withoutQuotes  = false;

      switch (valueType)
      {
      case ValueTypeString:
        effectiveValue = stringValue;
        break;

      case ValueTypeBoolean:
        effectiveValue = boolValue? "true" : "false";
        withoutQuotes  = true;
        break;

      case ValueTypeNumber:
        if (type == DATE_TYPE)
        {
          effectiveValue = isodate2str(numberValue);
        }
        else // regular number
        {
          effectiveValue = toString(numberValue);
          withoutQuotes  = true;
        }
        break;

      case ValueTypeNone:
        effectiveValue = "null";
        withoutQuotes  = true;
        break;

      default:
        LM_E(("Runtime Error (unknown value type: %d)", valueType));
      }

      out += valueTag2(indent + "  ", "value",
                              (request != RtUpdateContextResponse)? effectiveValue : "",
                              commaAfterContextValue, withoutQuotes);

    }
    else if (request == RtUpdateContextResponse)
    {
      out += valueTag1(indent + "  ", "value", "", commaAfterContextValue);
    }
  }
  else
  {
    bool isCompoundVector = false;

    if ((compoundValueP != NULL) && (compoundValueP->valueType == orion::ValueTypeVector))
    {
      isCompoundVector = true;
    }

    out += startTag2(indent + "  ", "value", isCompoundVector, true);
    out += compoundValueP->render(ciP, indent + "    ");
    out += endTag(indent + "  ", commaAfterContextValue, isCompoundVector);
  }

  out += metadataVector.render(indent + "  ", false);
  out += endTag(indent, comma);

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
std::string ContextAttribute::toJson
(
  bool                             isLastElement,
  RenderFormat                     renderFormat,
  const std::vector<std::string>&  metadataFilter,
  RequestType                      requestType
)
{
  std::string  out;

  // Add special metadata representing attribute dates
  if ((creDate != 0) && (std::find(metadataFilter.begin(), metadataFilter.end(), NGSI_MD_DATECREATED) != metadataFilter.end()))
  {
    Metadata* mdP = new Metadata(NGSI_MD_DATECREATED, DATE_TYPE, creDate);
    metadataVector.push_back(mdP);
  }
  if ((modDate != 0) && (std::find(metadataFilter.begin(), metadataFilter.end(), NGSI_MD_DATEMODIFIED) != metadataFilter.end()))
  {
    Metadata* mdP = new Metadata(NGSI_MD_DATEMODIFIED, DATE_TYPE, modDate);
    metadataVector.push_back(mdP);
  }

  if ((renderFormat == NGSI_V2_VALUES) || (renderFormat == NGSI_V2_KEYVALUES) || (renderFormat == NGSI_V2_UNIQUE_VALUES))
  {
    out = (renderFormat == NGSI_V2_KEYVALUES)? JSON_STR(name) + ":" : "";

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
    // This is needed for entities coming from NGSIv1 (which allows empty or missing types)
    //
    std::string defType = defaultType(valueType);

    if (compoundValueP && compoundValueP->isVector())
    {
      defType = defaultType(orion::ValueTypeVector);
    }

    out += (type != "")? JSON_VALUE("type", type) : JSON_VALUE("type", defType);
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
    out += JSON_STR("metadata") + ":" + "{" + metadataVector.toJson(true, metadataFilter) + "}";

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

  if (compoundValueP == NULL)  // Not a compound - text/plain must be accepted
  {
    if (ciP->httpHeaders.accepted("text/plain"))
    {
      char buf[64];

      ciP->outMimeType = TEXT;

      switch (valueType)
      {
      case orion::ValueTypeString:
        if (ciP->apiVersion == "v2")
        { 
          out = '"' + stringValue + '"';
        }
        else
        { 
          out = stringValue;
        }
        break;

      case orion::ValueTypeNumber:
        if (type == DATE_TYPE)
        {
          out = isodate2str(numberValue);
        }
        else // regular number
        {          
          out = toString(numberValue);
        }
        break;

      case orion::ValueTypeBoolean:
        snprintf(buf, sizeof(buf), "%s", boolValue? "true" : "false");
        out = buf;
        break;

      case orion::ValueTypeNone:
        snprintf(buf, sizeof(buf), "%s", "null");
        out = buf;
        break;

      default:
        out = "ERROR";
        break;
      }
    }
    else
    {
      OrionError oe(SccNotAcceptable, "accepted MIME types: text/plain", "NotAcceptable");
      ciP->httpStatusCode = SccNotAcceptable;

      out = oe.toJson();
    }
  }
  else if (compoundValueP != NULL)  /// Compound: application/json OR text/plain must be accepted
  {
    if (!ciP->httpHeaders.accepted("application/json") && !ciP->httpHeaders.accepted("text/plain"))
    {
      OrionError oe(SccNotAcceptable, "accepted MIME types: application/json, text/plain", "NotAcceptable");
      ciP->httpStatusCode = SccNotAcceptable;

      out = oe.toJson();
    }
    else
    {
      ciP->outMimeType = ciP->httpHeaders.outformatSelect();

      if (compoundValueP->isVector())
      {
        out = "[" + compoundValueP->toJson(true) + "]";
      }
      else  // Object
      {
        out = "{" + compoundValueP->toJson(false) + "}";
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
  const std::string&  indent,
  const std::string&  predetectedError,
  int                 counter
)
{
  size_t len;
  char errorMsg[128];

  if (((ciP->apiVersion == "v2") && (len = strlen(name.c_str())) < MIN_ID_LEN))
  {
    snprintf(errorMsg, sizeof errorMsg, "attribute name length: %zd, min length supported: %d", len, MIN_ID_LEN);
    alarmMgr.badInput(clientIp, errorMsg);
    return std::string(errorMsg);
  }

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


  if (ciP->apiVersion == "v2" && (requestType != EntityAttributeValueRequest) && (len = strlen(type.c_str())) < MIN_ID_LEN)
  {
    snprintf(errorMsg, sizeof errorMsg, "attribute type length: %zd, min length supported: %d", len, MIN_ID_LEN);
    alarmMgr.badInput(clientIp, errorMsg);
    return std::string(errorMsg);
  }

  if ((requestType != EntityAttributeValueRequest) && forbiddenIdChars(ciP->apiVersion, type.c_str()))
  {
    alarmMgr.badInput(clientIp, "found a forbidden character in the type of an attribute");
    return "Invalid characters in attribute type";
  }

  if ((compoundValueP != NULL) && (compoundValueP->childV.size() != 0))
  {
    return compoundValueP->check();
  }

  if (valueType == orion::ValueTypeString)
  {
    if (forbiddenChars(stringValue.c_str()))
    {
      alarmMgr.badInput(clientIp, "found a forbidden character in the value of an attribute");
      return "Invalid characters in attribute value";
    }
  }

  return metadataVector.check(ciP, requestType, indent + "  ", predetectedError, counter);
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
  LM_T(LmtPresent, ("%s  creDate:   %f",
        creDate,
        type.c_str()));
  LM_T(LmtPresent, ("%s  modDate:   %f",
        modDate,
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
		    mimeTypeToString(providingApplication.getMimeType())));
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
  if (previousValue != NULL)
  {
    previousValue->release();
    delete previousValue;
    previousValue = NULL;
  }
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
  switch (valueType)
  {
  case orion::ValueTypeString:
    return stringValue;
    break;

  case orion::ValueTypeNumber:
    return toString(numberValue);
    break;

  case orion::ValueTypeBoolean:
    return boolValue ? "true" : "false";
    break;

  case orion::ValueTypeNone:
    return "null";
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



/* ****************************************************************************
*
* ContextAttribute::compoundItemExists - 
*/
bool ContextAttribute::compoundItemExists(const std::string& compoundPath, orion::CompoundValueNode** compoundItemPP)
{
  std::vector<std::string>   compoundPathV;
  orion::CompoundValueNode*  current = compoundValueP;
  int                        levels;

  if (compoundPath == "")
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
      if (current->childV[cIx]->name == compoundPathV[ix])
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
