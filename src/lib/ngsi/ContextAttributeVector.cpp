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
#include <map>
#include <string>
#include <vector>

#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"

#include "common/globals.h"
#include "common/tag.h"
#include "common/string.h"
#include "common/RenderFormat.h"
#include "common/JsonHelper.h"
#include "ngsi/ContextAttributeVector.h"
#include "ngsi/Request.h"

#include "mongoBackend/MongoGlobal.h"
#include "mongoBackend/dbConstants.h"
#include "mongoBackend/dbFieldEncoding.h"
#include "mongoBackend/compoundResponses.h"
#include "mongoDriver/safeMongo.h"



/* ****************************************************************************
*
* ContextAttributeVector::ContextAttributeVector - 
*/
ContextAttributeVector::ContextAttributeVector()
{
  vec.clear();
}



/* ****************************************************************************
*
* ContextAttributeVector::toJsonTypes -
*
*/
std::string ContextAttributeVector::toJsonTypes(void)
{
  // Pass 1 - get per-attribute types
  std::map<std::string, std::map<std::string, int> > perAttrTypes;

  for (unsigned int ix = 0; ix < vec.size(); ++ix)
  {
    ContextAttribute* caP = vec[ix];
    perAttrTypes[caP->name][caP->type] = 1;   // just to mark that type exists
  }

  // Pass 2 - generate JSON
  JsonObjectHelper jh;

  std::map<std::string, std::map<std::string, int> >::iterator it;
  unsigned int                                                 ix;
  for (it = perAttrTypes.begin(), ix = 0; it != perAttrTypes.end(); ++it, ++ix)
  {
    std::string                 attrName  = it->first;
    std::map<std::string, int>  attrTypes = it->second;

    JsonVectorHelper jvh;

    std::map<std::string, int>::iterator jt;
    unsigned int                         jx;

    JsonObjectHelper jhTypes;

    for (jt = attrTypes.begin(), jx = 0; jt != attrTypes.end(); ++jt, ++jx)
    {
      std::string type = jt->first;
      
      //
      // Special condition for 'options=noAttrDetail':
      //   When the 'options' URI parameter contains 'noAttrDetail',
      //   mongoBackend fills the attribute type vector with *just one item* (that is an empty string).
      //   This special condition is checked for here, to produce a [] for the vector for the response.
      //
      // See the origin of this in mongoQueryTypes.cpp. Look for "NOTE: here we add", in two locations.
      //
      if ((!type.empty()) || (attrTypes.size() != 1))
      {
        jvh.addString(type);
      }

    }

    jhTypes.addRaw("types", jvh.str());

    jh.addRaw(attrName, jhTypes.str());
  }

  return jh.str();
}



/* ****************************************************************************
*
* ContextAttributeVector::toJsonV1 -
*
* FIXME P5: this method doesn't depend on the class object. Should be moved out of the class?
* Maybe included in the Entiy class render logic.
*/
std::string ContextAttributeVector::toJsonV1
(  
  bool                                   asJsonObject,
  RequestType                            request,
  const std::vector<ContextAttribute*>&  orderedAttrs,
  const std::vector<std::string>&        metadataFilter,
  bool                                   comma,
  bool                                   omitValue,
  bool                                   attrsAsName
)
{
  std::string out = "";

  if (orderedAttrs.size() == 0)
  {
    return "";
  }

  //
  // NOTE:
  // If the URI parameter 'attributeFormat' is set to 'object', then the attribute vector
  // is to be rendered as objects for JSON, and not as a vector.
  //
  if (asJsonObject)
  {
    // Note that in the case of attribute as name, we have to use a vector, thus using
    // attrsAsName variable as value for isVector parameter
    out += startTag("attributes", attrsAsName);
    for (unsigned int ix = 0; ix < orderedAttrs.size(); ++ix)
    {
      bool comma = (ix != orderedAttrs.size() -1);
      if (attrsAsName)
      {
        out += orderedAttrs[ix]->toJsonV1AsNameString(comma);
      }
      else
      {
        out += orderedAttrs[ix]->toJsonV1(asJsonObject, request, metadataFilter, comma, omitValue);
      }
    }   
    out += endTag(comma, attrsAsName);
  }
  else
  {
    out += startTag("attributes", true);
    for (unsigned int ix = 0; ix < orderedAttrs.size(); ++ix)
    {
      if (attrsAsName)
      {
        out += orderedAttrs[ix]->toJsonV1AsNameString(ix != orderedAttrs.size() - 1);
      }
      else
      {
        out += orderedAttrs[ix]->toJsonV1(asJsonObject, request, metadataFilter, ix != orderedAttrs.size() - 1, omitValue);
      }
    }
    out += endTag(comma, true);
  }

  return out;
}



/* ****************************************************************************
*
* ContextAttributeVector::check - 
*/
std::string ContextAttributeVector::check(ApiVersion apiVersion, RequestType requestType)
{
  for (unsigned int ix = 0; ix < vec.size(); ++ix)
  {
    std::string res;

    if ((res = vec[ix]->check(apiVersion, requestType)) != "OK")
      return res;
  }

  return "OK";
}



/* ****************************************************************************
*
* ContextAttributeVector::push_back -
*
*/
void ContextAttributeVector::push_back(ContextAttribute* item)
{
  vec.push_back(item);
}



/* ****************************************************************************
*
* ContextAttributeVector::push_back - 
*
*/
void ContextAttributeVector::push_back(const ContextAttributeVector& caV, bool cloneCompound)
{
  for (unsigned int ix = 0; ix < caV.size(); ++ix)
  {
    vec.push_back(new ContextAttribute(caV[ix], false, cloneCompound));
  }
}



/* ****************************************************************************
*
* ContextAttributeVector::size - 
*/
unsigned int ContextAttributeVector::size(void) const
{
  return vec.size();
}


/* ****************************************************************************
*
* ContextAttributeVector::operator[] -
*/
ContextAttribute*  ContextAttributeVector::operator[](unsigned int ix) const
{
  if (ix < vec.size())
  {
    return vec[ix];
  }
  return NULL;
}



/* ****************************************************************************
*
* ContextAttributeVector::release -
*/
void ContextAttributeVector::release(void)
{
  for (unsigned int ix = 0; ix < vec.size(); ++ix)
  {
    vec[ix]->release();
    delete vec[ix];
  }

  vec.clear();
}



/* ****************************************************************************
*
* ContextAttributeVector::fill - 
*
*/
void ContextAttributeVector::fill(const ContextAttributeVector& caV, bool useDefaultType, bool cloneCompounds)
{
  if (caV.size() == 0)
  {
    return;
  }

  for (unsigned int ix = 0; ix < caV.size(); ++ix)
  {
    ContextAttribute* from = caV[ix];
    ContextAttribute* caP = new ContextAttribute(from, useDefaultType, cloneCompounds);

    push_back(caP);
  }
}



/* ****************************************************************************
*
* ContextAttributeVector::fill -
*/
void ContextAttributeVector::fill
(
  const orion::BSONObj&  attrs,
  const StringList&      attrL,
  bool                   includeEmpty,
  const std::string&     locAttr,
  ApiVersion             apiVersion
)
{
  std::set<std::string>  attrNames;

  attrs.getFieldNames(&attrNames);
  for (std::set<std::string>::iterator i = attrNames.begin(); i != attrNames.end(); ++i)
  {
    std::string        attrName                = *i;
    orion::BSONObj     attr                    = getObjectFieldF(attrs, attrName);
    ContextAttribute*  caP                     = NULL;
    ContextAttribute   ca;
    bool               noLocationMetadata      = true;

    // Name and type
    ca.name           = dbDecode(attrName);
    ca.type           = getStringFieldF(attr, ENT_ATTRS_TYPE);

    // Skip attribute if the attribute is in the list (or attrL is empty or includes "*")
    if (!includedAttribute(ca.name, attrL))
    {
      continue;
    }

    /* It could happen (although very rarely) that the value field is missing in the
     * DB for the attribute. The following is a safety check measure to protect against that */
    if (!attr.hasField(ENT_ATTRS_VALUE))
    {
      caP = new ContextAttribute(ca.name, ca.type, "");
    }
    else
    {
      switch(getFieldF(attr, ENT_ATTRS_VALUE).type())
      {
      case orion::String:
        ca.stringValue = getStringFieldF(attr, ENT_ATTRS_VALUE);
        if (!includeEmpty && ca.stringValue.empty())
        {
          continue;
        }
        caP = new ContextAttribute(ca.name, ca.type, ca.stringValue);
        break;

      case orion::NumberDouble:
        ca.numberValue = getNumberFieldF(attr, ENT_ATTRS_VALUE);
        caP = new ContextAttribute(ca.name, ca.type, ca.numberValue);
        break;

      case orion::NumberInt:
        ca.numberValue = (double) getIntFieldF(attr, ENT_ATTRS_VALUE);
        caP = new ContextAttribute(ca.name, ca.type, ca.numberValue);
        break;

      case orion::Bool:
        ca.boolValue = getBoolFieldF(attr, ENT_ATTRS_VALUE);
        caP = new ContextAttribute(ca.name, ca.type, ca.boolValue);
        break;

      case orion::jstNULL:
        caP = new ContextAttribute(ca.name, ca.type, "");
        caP->valueType = orion::ValueTypeNull;
        break;

      case orion::Object:
        caP = new ContextAttribute(ca.name, ca.type, "");
        caP->compoundValueP = new orion::CompoundValueNode(orion::ValueTypeObject);
        caP->valueType = orion::ValueTypeObject;
        compoundObjectResponse(caP->compoundValueP, getFieldF(attr, ENT_ATTRS_VALUE));
        break;

      case orion::Array:
        caP = new ContextAttribute(ca.name, ca.type, "");
        caP->compoundValueP = new orion::CompoundValueNode(orion::ValueTypeVector);
        caP->valueType = orion::ValueTypeVector;
        compoundVectorResponse(caP->compoundValueP, getFieldF(attr, ENT_ATTRS_VALUE));
        break;

      default:
        LM_E(("Runtime Error (unknown attribute value type in DB: %d)", getFieldF(attr, ENT_ATTRS_VALUE).type()));
      }
    }

    /* dateExpires is managed like a regular attribute in DB, but it is a builtin and it is shadowed */
    if (caP->name == DATE_EXPIRES)
    {
      caP->shadowed = true;
    }

    /* Setting custom metadata (if any) */
    if (attr.hasField(ENT_ATTRS_MD))
    {
      orion::BSONObj                mds = getObjectFieldF(attr, ENT_ATTRS_MD);
      std::set<std::string>  mdsSet;

      mds.getFieldNames(&mdsSet);
      for (std::set<std::string>::iterator i = mdsSet.begin(); i != mdsSet.end(); ++i)
      {
        std::string currentMd = *i;
        Metadata*   md = new Metadata(dbDecode(currentMd), getObjectFieldF(mds, currentMd));

        /* The flag below indicates that a location metadata with WGS84 was found during iteration.
        *  It needs to the NGSIV1 check below, in order to add it if the flag is false
        *  In addition, adjust old wrong WSG84 metadata value with WGS84 */
        if (md->name == NGSI_MD_LOCATION)
        {
          noLocationMetadata = false;

          if (md->valueType == orion::ValueTypeString && md->stringValue == LOCATION_WGS84_LEGACY)
          {
            md->stringValue = LOCATION_WGS84;
          }
        }

        caP->metadataVector.push_back(md);
      }
    }

    if (apiVersion == V1)
    {
      /* Setting location metadata (if location attr found
       *  and the location metadata was not present or was present but with old wrong WSG84 value) */
      if ((locAttr == ca.name) && (ca.type != GEO_POINT) && noLocationMetadata)
      {
        /* Note that if attribute type is geo:point then the user is using the "new way"
         * of locating entities in NGSIv1, thus location metadata is not rendered */
        Metadata* md = new Metadata(NGSI_MD_LOCATION, "string", LOCATION_WGS84);
        caP->metadataVector.push_back(md);
      }
    }

    /* Set creDate and modDate at attribute level */
    if (attr.hasField(ENT_ATTRS_CREATION_DATE))
    {
      caP->creDate = getNumberFieldF(attr, ENT_ATTRS_CREATION_DATE);
    }

    if (attr.hasField(ENT_ATTRS_MODIFICATION_DATE))
    {
      caP->modDate = getNumberFieldF(attr, ENT_ATTRS_MODIFICATION_DATE);
    }

    this->push_back(caP);
  }

}



/* ****************************************************************************
*
* ContextAttributeVector::fill -
*
* Wrapper of BSON-based fill with less parameters
*/
void ContextAttributeVector::fill(const orion::BSONObj&  attrs)
{
  StringList emptyList;
  return fill(attrs, emptyList, true, "", V2);
}



/* ****************************************************************************
*
* ContextAttributeVector::get -
*/
int ContextAttributeVector::get(const std::string& attributeName) const
{
  for (unsigned int ix = 0; ix < vec.size(); ++ix)
  {
    if (vec[ix]->name == attributeName)
    {
      return ix;
    }
  }

  return -1;
}



/* ****************************************************************************
*
* ContextAttributeVector::toBson -
*/
void ContextAttributeVector::toBson
(
  double                    now,
  orion::BSONObjBuilder*    attrsToAdd,
  orion::BSONArrayBuilder*  attrNamesToAdd,
  ApiVersion                apiVersion
) const
{
  for (unsigned int ix = 0; ix < this->vec.size(); ++ix)
  {
    orion::BSONObjBuilder  bsonAttr;

    std::string attrType;

    if (!this->vec[ix]->typeGiven && (apiVersion == V2))
    {
      if ((this->vec[ix]->compoundValueP == NULL) || (this->vec[ix]->compoundValueP->valueType != orion::ValueTypeVector))
      {
        attrType = defaultType(this->vec[ix]->valueType);
      }
      else
      {
        attrType = defaultType(orion::ValueTypeVector);
      }
    }
    else
    {
      attrType = this->vec[ix]->type;
    }

    bsonAttr.append(ENT_ATTRS_TYPE, attrType);

    // negative values in now are used in case we don't want creation and modification date
    // fields (typically in the ngsi field in custom notifications)
    if (now >= 0)
    {
      bsonAttr.append(ENT_ATTRS_CREATION_DATE, now);
      bsonAttr.append(ENT_ATTRS_MODIFICATION_DATE, now);
    }

    this->vec[ix]->valueBson(std::string(ENT_ATTRS_VALUE), &bsonAttr, attrType, ngsiv1Autocast && (apiVersion == V1));

    std::string effectiveName = dbEncode(this->vec[ix]->name);

    LM_T(LmtMongo, ("new attribute: {name: %s, type: %s, value: %s}",
                    effectiveName.c_str(),
                    this->vec[ix]->type.c_str(),
                    this->vec[ix]->getValue().c_str()));

    /* Custom metadata */
    orion::BSONObjBuilder    md;
    orion::BSONArrayBuilder  mdNames;

    this->vec[ix]->metadataVector.toBson(&md, &mdNames, apiVersion == V2);
    if (mdNames.arrSize())
    {
      bsonAttr.append(ENT_ATTRS_MD, md.obj());
    }
    bsonAttr.append(ENT_ATTRS_MDNAMES, mdNames.arr());

    attrsToAdd->append(effectiveName, bsonAttr.obj());
    attrNamesToAdd->append(this->vec[ix]->name);
  }
}
