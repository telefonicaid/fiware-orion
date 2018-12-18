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
#include "common/JsonHelper.h"
#include "alarmMgr/alarmMgr.h"
#include "orionTypes/OrionValueType.h"
#include "parse/forbiddenChars.h"
#include "ngsi/ContextAttribute.h"
#include "rest/HttpStatusCode.h"
#include "rest/OrionError.h"
#include "parse/CompoundValueNode.h"

#include "mongo/client/dbclient.h"
#include "mongoBackend/dbConstants.h"
#include "mongoBackend/dbFieldEncoding.h"
#include "mongoBackend/compoundValueBson.h"
#include "mongoBackend/dbFieldEncoding.h"

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
* ContextAttribute::~ContextAttribute - 
*/
ContextAttribute::~ContextAttribute()
{
  release();
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
  numberValue           = 0;
  valueType             = orion::ValueTypeNotGiven;
  compoundValueP        = NULL;
  found                 = false;
  skip                  = false;
  typeGiven             = false;
  onlyValue             = false;
  previousValue         = NULL;
  actionType            = "";
  shadowed              = false;

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
ContextAttribute::ContextAttribute(ContextAttribute* caP, bool useDefaultType, bool cloneCompound)
{
  CompoundValueNode* cvnP = NULL;
  if (cloneCompound)
  {
    if (caP->compoundValueP != NULL)
    {
      cvnP = caP->compoundValueP->clone();
    }
  }

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
  actionType            = caP->actionType;
  shadowed              = caP->shadowed;

#if 0
  // This block seems to be a better alternative to implement cloneCompound. If enabled
  // the "if (cloneCompound)" fragments at the start/end of the method should be removed
  // and also the following:
  //
  //   compoundValueP        = caP->compoundValueP;
  //   caP->compoundValueP   = NULL;
  //
  // However, if enabled I get segfault (it seems that due to double-free). However, I don't
  // understand the reason... the code seems to be equivalent to the current alternative.
  //
  // FIXME P8: this may be related with #3162

  if (cloneCompound)
  {
    if (caP->compoundValueP != NULL)
    {
      compoundValueP    = caP->compoundValueP->clone();
    }
  }
  else
  {
    compoundValueP      = caP->compoundValueP;
    caP->compoundValueP = NULL;
  }
#endif

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

  if (cloneCompound)
  {
    if (cvnP != NULL)
    {
      caP->compoundValueP = cvnP;
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
  numberValue           = 0;
  valueType             = orion::ValueTypeString;
  compoundValueP        = NULL;
  found                 = _found;
  skip                  = false;
  typeGiven             = false;
  onlyValue             = false;
  previousValue         = NULL;
  actionType            = "";
  shadowed              = false;

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
  numberValue           = 0;
  valueType             = orion::ValueTypeString;
  compoundValueP        = NULL;
  found                 = _found;
  skip                  = false;
  typeGiven             = false;
  onlyValue             = false;
  previousValue         = NULL;
  actionType            = "";
  shadowed              = false;

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
  actionType            = "";
  shadowed              = false;

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
  numberValue           = 0;
  valueType             = orion::ValueTypeBoolean;
  compoundValueP        = NULL;
  found                 = _found;
  skip                  = false;
  typeGiven             = false;
  previousValue         = NULL;
  actionType            = "";
  shadowed              = false;

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
  numberValue           = 0;
  found                 = false;
  valueType             = orion::ValueTypeObject;  // FIXME P6: Could be ValueTypeVector ...
  skip                  = false;
  typeGiven             = false;
  previousValue         = NULL;
  actionType            = "";
  shadowed              = false;

  creDate = 0;
  modDate = 0;

  providingApplication.set("");
  providingApplication.setMimeType(NOMIMETYPE);
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
* toJsonV1AsObject -
*/
std::string ContextAttribute::toJsonV1AsObject
(
  RequestType                    request,
  const std::vector<Metadata*>&  orderedMetadata,
  bool                           comma,
  bool                           omitValue
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
          effectiveValue = double2string(numberValue);
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
    out += JSON_STR("value") + ":" + compoundValueP->toJson();
  }

  if (omitValue == false)
  {
    out += metadataVector.toJsonV1(orderedMetadata, false);
  }

  out += endTag(comma);

  return out;
}



/* ****************************************************************************
*
* toJsonV1AsNameString -
*/
std::string ContextAttribute::toJsonV1AsNameString(bool comma)
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
* toJsonV1 -
*/
std::string ContextAttribute::toJsonV1
(
  bool                             asJsonObject,
  RequestType                      request,
  const std::vector<std::string>&  metadataFilter,
  bool                             comma,
  bool                             omitValue
)
{
  // Filter and order metadata
  std::vector<Metadata*> orderedMetadata;
  filterAndOrderMetadata(metadataFilter, &orderedMetadata);

  std::string  out                    = "";
  bool         valueRendered          = (compoundValueP != NULL) || (omitValue == false) || (request == RtUpdateContextResponse);
  bool         commaAfterContextValue = orderedMetadata.size() != 0;
  bool         commaAfterType         = valueRendered;

  if (asJsonObject)
  {
    return toJsonV1AsObject(request, orderedMetadata, comma, omitValue);
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
          effectiveValue = double2string(numberValue);
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
    out += JSON_STR("value") + ":" + compoundValueP->toJson();

    if (commaAfterContextValue)
    {
      out += ',';
    }
  }

  out += metadataVector.toJsonV1(orderedMetadata, false);
  out += endTag(comma);

  return out;
}


/* ****************************************************************************
*
* ContextAttribute::filterAndOrderMetadata -
*
*/
void ContextAttribute::filterAndOrderMetadata
(
  const std::vector<std::string>&  metadataFilter,
  std::vector<Metadata*>* orderedMetadata
)
{
  if (metadataFilter.size() == 0)
  {
    // No filter. Metadata are "as is" in the attribute except shadowed ones,
    // which require explicit inclusiong (dateCreated, etc.)
    for (unsigned int ix = 0; ix < metadataVector.size(); ix++)
    {
      if (!metadataVector[ix]->shadowed)
      {
        orderedMetadata->push_back(metadataVector[ix]);
      }
    }
  }
  else
  {
    // Filter. Processing will depend on whether '*' is in the metadataFilter or not
    if (std::find(metadataFilter.begin(), metadataFilter.end(), NGSI_MD_ALL) != metadataFilter.end())
    {
      // If '*' is in: all metadata are included in the same order used by the entity

      for (unsigned int ix = 0; ix < metadataVector.size(); ix++)
      {
        if (metadataVector[ix]->shadowed)
        {
          // Shadowed metadata need explicit inclusion
          if ((std::find(metadataFilter.begin(), metadataFilter.end(), metadataVector[ix]->name) != metadataFilter.end()))
          {
            orderedMetadata->push_back(metadataVector[ix]);
          }
        }
        else
        {
          orderedMetadata->push_back(metadataVector[ix]);
        }
      }
    }
    else
    {
      // - If '*' is not in: metadata are include in the metadataFilter order

      for (unsigned int ix = 0; ix < metadataFilter.size(); ix++)
      {
        Metadata* mdP;
        if ((mdP = metadataVector.lookupByName(metadataFilter[ix])) != NULL)
        {
          orderedMetadata->push_back(mdP);
        }
      }
    }
  }
}



/* ****************************************************************************
*
* toJson -
*
*/
std::string ContextAttribute::toJson(const std::vector<std::string>&  metadataFilter)
{
  JsonObjectHelper jh;

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

  jh.addString("type", type != ""? type : defType);

  //
  // value
  //
  if (compoundValueP != NULL)
  {
    jh.addRaw("value", compoundValueP->toJson());
  }
  else if (valueType == orion::ValueTypeNumber)
  {
    if ((type == DATE_TYPE) || (type == DATE_TYPE_ALT))
    {
      jh.addString("value", isodate2str(numberValue));
    }
    else // regular number
    {
      jh.addNumber("value", numberValue);
    }
  }
  else if (valueType == orion::ValueTypeString)
  {
    jh.addString("value", stringValue);
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
    LM_E(("Runtime Error (value not given for attribute %s)", name.c_str()));
  }
  else
  {
    LM_E(("Runtime Error (invalid value type %s for attribute %s)", valueTypeName(valueType), name.c_str()));
  }

  std::vector<Metadata*> orderedMetadata;
  filterAndOrderMetadata(metadataFilter, &orderedMetadata);

  //
  // metadata
  //
  jh.addRaw("metadata", metadataVector.toJson(orderedMetadata));

  return jh.str();
}


/* ****************************************************************************
*
* toJsonValue -
*
* To be used by options=values and options=unique renderings
*
*/
std::string ContextAttribute::toJsonValue(void)
{
  if (compoundValueP != NULL)
  {
    return compoundValueP->toJson();
  }
  else if (valueType == orion::ValueTypeNumber)
  {
    if ((type == DATE_TYPE) || (type == DATE_TYPE_ALT))
    {
      std::string out = "\"";
      out += toJsonString(isodate2str(numberValue));
      out += '"';
      return out;
    }
    else // regular number
    {
      return double2string(numberValue);
    }
  }
  else if (valueType == orion::ValueTypeString)
  {
    std::string out = "\"";
    out += toJsonString(stringValue);
    out += '"';
    return out;
  }
  else if (valueType == orion::ValueTypeBoolean)
  {
    return boolValue ? "true" : "false";
  }
  else if (valueType == orion::ValueTypeNull)
  {
    return "null";
  }
  else if (valueType == orion::ValueTypeNotGiven)
  {
    LM_E(("Runtime Error (value not given for attribute %s)", name.c_str()));
  }
  else
  {
    LM_E(("Runtime Error (invalid value type %s for attribute %s)", valueTypeName(valueType), name.c_str()));
  }

  return "";
}


/* ****************************************************************************
*
* toJsonAsValue -
*
* FIXME P7: toJsonValue() and toJsonAsValue() are very similar and may be confusing.
* Try to find a couple of names different and meaningful enough
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
          out = double2string(numberValue);
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

      out = compoundValueP->toJson();
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
    return compoundValueP->check("");
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
    return double2string(numberValue);
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

    for (unsigned int cIx = 0; cIx < current->childV.size(); ++cIx)
    {
      if (dbDotEncode(current->childV[cIx]->name) == compoundPathV[ix])
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
