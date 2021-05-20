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
#include "rest/HttpStatusCode.h"
#include "rest/OrionError.h"
#include "parse/CompoundValueNode.h"

#include "mongo/client/dbclient.h"
#include "mongoBackend/dbConstants.h"
#include "orionld/common/eqForDot.h"
#include "mongoBackend/compoundValueBson.h"


using namespace mongo;
using namespace orion;



/* ****************************************************************************
*
* ContextAttribute::bsonAppendAttrValue -
*
*/
void ContextAttribute::bsonAppendAttrValue(BSONObjBuilder& bsonAttr, const std::string& attrType, bool autocast) const
{
  std::string effectiveStringValue = stringValue;
  bool        effectiveBoolValue   = boolValue;
  double      effectiveNumberValue = numberValue;
  ValueType   effectiveValueType   = valueType;

  // Checking for ValueTypeString is an additional safety measure (ensuring that the attribute came from NGSIv1 in plain text)
  if ((autocast) && (effectiveValueType == ValueTypeString))
  {
    // Autocast only for selected attribute types
    if ((attrType == DEFAULT_ATTR_NUMBER_TYPE) || (attrType == NUMBER_TYPE_ALT))
    {
      if (str2double(effectiveStringValue.c_str(), &effectiveNumberValue))
      {
        effectiveValueType = ValueTypeNumber;
      }
      // Note that if str2double() fails, we keep ValueTypeString and everything works like without autocast
    }
    if (attrType == DEFAULT_ATTR_BOOL_TYPE)
    {
      // Note that we cannot use isTrue() or isFalse() functions, as they consider also 0 and 1 as
      // valid true/false values and JSON spec mandates exactly true or false
      if (effectiveStringValue == "true")
      {
        effectiveBoolValue = true;
        effectiveValueType = ValueTypeBoolean;
      }
      else if (effectiveStringValue == "false")
      {
        effectiveBoolValue = false;
        effectiveValueType = ValueTypeBoolean;
      }
      // Note that if above checks fail, we keep ValueTypeString and everything works like without autocast
    }
    if ((attrType == DATE_TYPE) || (attrType == DATE_TYPE_ALT))
    {
      effectiveNumberValue = parse8601Time(effectiveStringValue);
      if (effectiveNumberValue != -1)
      {
        effectiveValueType = ValueTypeNumber;
      }
      // Note that if parse8601Time() fails, we keep ValueTypeString and everything works like without autocast
    }
  }

  switch (effectiveValueType)
  {
    case ValueTypeString:
      bsonAttr.append(ENT_ATTRS_VALUE, effectiveStringValue);
      break;

    case ValueTypeNumber:
      bsonAttr.append(ENT_ATTRS_VALUE, effectiveNumberValue);
      break;

    case ValueTypeBoolean:
      bsonAttr.append(ENT_ATTRS_VALUE, effectiveBoolValue);
      break;

    case ValueTypeNull:
      bsonAttr.appendNull(ENT_ATTRS_VALUE);
      break;

    case ValueTypeNotGiven:
      LM_E(("Runtime Error (value not given in compound value)"));
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
void ContextAttribute::valueBson(BSONObjBuilder& bsonAttr, const std::string& attrType, bool autocast) const
{
  if (compoundValueP == NULL)
  {
    bsonAppendAttrValue(bsonAttr, attrType, autocast);
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
    else if (compoundValueP->valueType == ValueTypeNull)
    {
      // FIXME P4: this is somehow redundant. See https://github.com/telefonicaid/fiware-orion/issues/271
      bsonAttr.appendNull(ENT_ATTRS_VALUE);
    }
    else if (compoundValueP->valueType == ValueTypeNotGiven)
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
* ContextAttribute::ContextAttribute - 
*/
ContextAttribute::ContextAttribute()
{
  name                  = "";
  type                  = "";
  stringValue           = "";
  numberValue           = 0;
  valueType             = orion::ValueTypeNotGiven;
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

  // Cloning metadata
  for (unsigned int mIx = 0; mIx < caP->metadataVector.size(); ++mIx)
  {
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
  name                  = _name;
  type                  = _type;
  stringValue           = std::string(_value);
  numberValue           = 0;
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
  name                  = _name;
  type                  = _type;
  stringValue           = _value;
  numberValue           = 0;
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
  name                  = _name;
  type                  = _type;
  boolValue             = _value;
  numberValue           = 0;
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
  name                  = _name;
  type                  = _type;
  compoundValueP        = _compoundValueP->clone();
  numberValue           = 0;
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
* ContextAttribute::getMetadataId -
*/
const char* ContextAttribute::getMetadataId() const
{
  for (unsigned int ix = 0; ix < metadataVector.size(); ++ix)
  {
    const char* mdName = metadataVector[ix]->name.c_str();

    if ((mdName[0] == 'I') && (mdName[1] == 'D') && (mdName[2] == 0))
    {
      return metadataVector[ix]->stringValue.c_str();
    }
  }

  return NULL;
}


/* ****************************************************************************
*
* ContextAttribute::getLocation() -
*/
std::string ContextAttribute::getLocation(ApiVersion apiVersion) const
{
  if (apiVersion == V1)
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
  ApiVersion   apiVersion,
  RequestType  request,
  bool         comma,
  bool         omitValue
)
{
  std::string  out                    = "";
  bool         commaAfterContextValue = metadataVector.size() != 0;
  bool         commaAfterType         = !omitValue || commaAfterContextValue;

  out += startTag(name, false);
  out += valueTag("type",         type,  commaAfterType);

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
        if ((type == DATE_TYPE) || (type == DATE_TYPE_ALT))
        {
          effectiveValue = isodate2str(numberValue);
        }
        else // regular number
        {
          effectiveValue = toString(numberValue);
          withoutQuotes  = true;
        }
        break;

      case ValueTypeNull:
        effectiveValue = "null";
        withoutQuotes  = true;
        break;

      case ValueTypeNotGiven:
        LM_E(("Runtime Error (value not given in compound value)"));
        break;

      default:
        LM_E(("Runtime Error (unknown value type: %d)", valueType));
      }

      //
      // NOTE
      // renderAsJsonObject is used in v1 only.
      // => we only need to care about stringValue (not boolValue, numberValue nor nullValue)
      //
      out += valueTag("value",
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

    out += startTag("value", isCompoundVector);
    out += compoundValueP->render(apiVersion);
    out += endTag(commaAfterContextValue, isCompoundVector);
  }

  if (omitValue == false)
  {
    out += metadataVector.render(false);
  }

  out += endTag(comma);

  return out;
}



/* ****************************************************************************
*
* renderAsNameString -
*/
std::string ContextAttribute::renderAsNameString(bool comma)
{
  std::string  out = "";

  if (comma)
  {
    out += "\"" + name + "\",";
  }
  else
  {
    out += "\"" + name + "\"";
  }

  return out;

}



/* ****************************************************************************
*
* render - 
*/
std::string ContextAttribute::render
(
  ApiVersion   apiVersion,
  bool         asJsonObject,
  RequestType  request,
  bool         comma,
  bool         omitValue
)
{
  std::string  out                    = "";
  bool         valueRendered          = (compoundValueP != NULL) || (omitValue == false) || (request == RtUpdateContextResponse);
  bool         commaAfterContextValue = metadataVector.size() != 0;
  bool         commaAfterType         = valueRendered;

  if (asJsonObject)
  {
    return renderAsJsonObject(apiVersion, request, comma, omitValue);
  }

  out += startTag();
  out += valueTag("name", name,  true);  // attribute.type is always rendered
  out += valueTag("type", type,  commaAfterType);

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
        if ((type == DATE_TYPE) || (type == DATE_TYPE_ALT))
        {
          effectiveValue = isodate2str(numberValue);
        }
        else // regular number
        {
          effectiveValue = toString(numberValue);
          withoutQuotes  = true;
        }
        break;

      case ValueTypeNull:
        effectiveValue = "null";
        withoutQuotes  = true;
        break;

      case ValueTypeNotGiven:
        LM_E(("Runtime Error (value not given in compound value)"));
        break;

      default:
        LM_E(("Runtime Error (unknown value type: %d)", valueType));
      }

      out += valueTag("value",
                      (request != RtUpdateContextResponse)? effectiveValue : "",
                      commaAfterContextValue,
                      false,
                      withoutQuotes);

    }
    else if (request == RtUpdateContextResponse)
    {
      out += valueTag("value", "", commaAfterContextValue);
    }
  }
  else
  {
    bool isCompoundVector = false;

    if ((compoundValueP != NULL) && (compoundValueP->valueType == orion::ValueTypeVector))
    {
      isCompoundVector = true;
    }

    out += startTag("value", isCompoundVector);
    out += compoundValueP->render(apiVersion);
    out += endTag(commaAfterContextValue, isCompoundVector);
  }

  out += metadataVector.render(false);
  out += endTag(comma);

  return out;
}



/* ****************************************************************************
*
* toJson -
*
* FIXME: Refactor this method in order to simplify the code paths of the rendering process
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
    // Lookup Metadata NGSI_MD_DATECREATED
    Metadata* dateCreatedMetadataP = NULL;
    for (unsigned int mIx = 0; mIx < metadataVector.size(); mIx++)
    {
      Metadata* mdP = metadataVector[mIx];

      if (mdP->name == NGSI_MD_DATECREATED)
      {
        dateCreatedMetadataP = mdP;
        break;
      }
    }

    if (dateCreatedMetadataP == NULL)
    {
      Metadata* mdP = new Metadata(NGSI_MD_DATECREATED, DATE_TYPE, creDate);
      metadataVector.push_back(mdP);
    }
    else
    {
      dateCreatedMetadataP->numberValue = creDate;
    }
  }

  if ((modDate != 0) && (std::find(metadataFilter.begin(), metadataFilter.end(), NGSI_MD_DATEMODIFIED) != metadataFilter.end()))
  {
    // Lookup Metadata NGSI_MD_DATEMODIFIED
    Metadata* dateModifiedMetadataP = NULL;
    for (unsigned int mIx = 0; mIx < metadataVector.size(); mIx++)
    {
      Metadata* mdP = metadataVector[mIx];

      if (mdP->name == NGSI_MD_DATEMODIFIED)
      {
        dateModifiedMetadataP = mdP;
        break;
      }
    }

    if (dateModifiedMetadataP == NULL)
    {
      Metadata* mdP = new Metadata(NGSI_MD_DATEMODIFIED, DATE_TYPE, modDate);
      metadataVector.push_back(mdP);
    }
    else
    {
      dateModifiedMetadataP->numberValue = modDate;
    }
  }

  if ((renderFormat == NGSI_V2_VALUES) || (renderFormat == NGSI_V2_KEYVALUES) || (renderFormat == NGSI_V2_UNIQUE_VALUES))
  {
    out = (renderFormat == NGSI_V2_KEYVALUES)? JSON_STR(name) + ":" : "";

    if (compoundValueP != NULL)
    {
      if (compoundValueP->isObject())
      {
        out += "{" + compoundValueP->toJson(true, true) + "}";
      }
      else if (compoundValueP->isVector())
      {
        out += "[" + compoundValueP->toJson(true, true) + "]";
      }
    }
    else if (valueType == orion::ValueTypeNumber)
    {
      if ((type == DATE_TYPE) || (type == DATE_TYPE_ALT))
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
    else if (valueType == orion::ValueTypeNull)
    {
      out += "null";
    }
    else if (valueType == orion::ValueTypeNotGiven)
    {
      LM_E(("Runtime Error (value not given in compound value)"));
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
        out += JSON_STR("value") + ":{" + compoundValueP->toJson(true, true) + "}";
      }
      else if (compoundValueP->isVector())
      {
        out += JSON_STR("value") + ":[" + compoundValueP->toJson(true, true) + "]";
      }
    }
    else if (valueType == orion::ValueTypeNumber)
    {
      if ((type == DATE_TYPE) || (type == DATE_TYPE_ALT))
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
    else if (valueType == orion::ValueTypeNull)
    {
      out += JSON_STR("value") + ":" + "null";
    }
    else if (valueType == orion::ValueTypeNotGiven)
    {
      LM_E(("Runtime Error (value not given in compound value)"));
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
std::string ContextAttribute::toJsonAsValue
(
  ApiVersion       apiVersion,          // in parameter
  bool             acceptedTextPlain,   // in parameter
  bool             acceptedJson,        // in parameter
  MimeType         outFormatSelection,  // in parameter
  MimeType*        outMimeTypeP,        // out parameter
  HttpStatusCode*  scP                  // out parameter
)
{
  std::string  out;

  if (compoundValueP == NULL)  // Not a compound - text/plain must be accepted
  {
    if (acceptedTextPlain)
    {
      char buf[64];

      *outMimeTypeP = TEXT;

      switch (valueType)
      {
      case orion::ValueTypeString:
        if (apiVersion == V2)
        { 
          out = '"' + stringValue + '"';
        }
        else
        { 
          out = stringValue;
        }
        break;

      case orion::ValueTypeNumber:
        if ((type == DATE_TYPE) || (type == DATE_TYPE_ALT))
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

      case orion::ValueTypeNull:
        snprintf(buf, sizeof(buf), "%s", "null");
        out = buf;
        break;

      case orion::ValueTypeNotGiven:
        LM_E(("Runtime Error (value not given in compound value)"));
        break;

      default:
        out = "ERROR";
        break;
      }
    }
    else
    {
      OrionError oe(SccNotAcceptable, "accepted MIME types: text/plain", "NotAcceptable");
      *scP = SccNotAcceptable;

      out = oe.toJson();
    }
  }
  else if (compoundValueP != NULL)  // Compound: application/json OR text/plain must be accepted
  {
    if (!acceptedJson && !acceptedTextPlain)
    {
      OrionError oe(SccNotAcceptable, "accepted MIME types: application/json, text/plain", "NotAcceptable");
      *scP = SccNotAcceptable;

      out = oe.toJson();
    }
    else
    {
      *outMimeTypeP = outFormatSelection;

      if (compoundValueP->isVector())
      {
        out = "[" + compoundValueP->toJson(true, true) + "]";
      }
      else  // Object
      {
        out = "{" + compoundValueP->toJson(false, true) + "}";
      }
    }
  }

  return out;
}



/* ****************************************************************************
*
* ContextAttribute::check - 
*/
std::string ContextAttribute::check(ApiVersion apiVersion, RequestType requestType)
{
  size_t len;
  char errorMsg[128];

  if (((apiVersion == V2) && (len = strlen(name.c_str())) < MIN_ID_LEN) && (requestType != EntityAttributeValueRequest))
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

  if (forbiddenIdChars(apiVersion, name.c_str()))
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


  if (apiVersion == V2 && (requestType != EntityAttributeValueRequest) && (len = strlen(type.c_str())) < MIN_ID_LEN)
  {
    snprintf(errorMsg, sizeof errorMsg, "attribute type length: %zd, min length supported: %d", len, MIN_ID_LEN);
    alarmMgr.badInput(clientIp, errorMsg);
    return std::string(errorMsg);
  }

  if ((requestType != EntityAttributeValueRequest) && forbiddenIdChars(apiVersion, type.c_str()))
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

  return metadataVector.check(apiVersion);
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

  case orion::ValueTypeNull:
    return "null";
    break;

  case orion::ValueTypeNotGiven:    
    return "<not given>";
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
    char compoundPathEncoded[256];

    strncpy(compoundPathEncoded, compoundPathV[ix].c_str(), sizeof(compoundPathEncoded));
    eqForDot(compoundPathEncoded);

    for (unsigned int cIx = 0; cIx < current->childV.size(); ++cIx)
    {
      if (strcmp(current->childV[cIx]->name.c_str(), compoundPathEncoded) == 0)
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
