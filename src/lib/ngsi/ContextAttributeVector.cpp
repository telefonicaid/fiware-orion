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
  const StringList&      attrL
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
        LM_E(("Runtime Error (unknown attribute value type in DB: %d on attribute %s)", getFieldF(attr, ENT_ATTRS_VALUE).type(), ca.name.c_str()));
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
  return fill(attrs, emptyList);
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

    // FIXME P7: boolean return value should be managed?
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



/* ****************************************************************************
*
* ContextAttributeVector::applyUpdateOperators -
*/
void ContextAttributeVector::applyUpdateOperators(void)
{
  std::vector<std::string> toErase;

  for (unsigned int ix = 0; ix < vec.size(); ++ix)
  {
    if (vec[ix]->compoundValueP != NULL)
    {
      if ((vec[ix]->compoundValueP->valueType == orion::ValueTypeObject) && (vec[ix]->compoundValueP->childV.size() > 0) && (isUpdateOperator(vec[ix]->compoundValueP->childV[0]->name)))
      {
        orion::CompoundValueNode* upOp = vec[ix]->compoundValueP->childV[0];
        std::string op = upOp->name;
        if (op == "$inc")
        {
          vec[ix]->valueType = orion::ValueTypeNumber;
          vec[ix]->numberValue = upOp->numberValue;
          delete vec[ix]->compoundValueP;
          vec[ix]->compoundValueP = NULL;
        }
        else if (op == "$min")
        {
          vec[ix]->valueType = orion::ValueTypeNumber;
          if (upOp->numberValue > 0)
          {
            vec[ix]->numberValue = 0;
          }
          else
          {
            vec[ix]->numberValue = upOp->numberValue;
          }
          delete vec[ix]->compoundValueP;
          vec[ix]->compoundValueP = NULL;
        }
        else if (op == "$max")
        {
          vec[ix]->valueType = orion::ValueTypeNumber;
          if (upOp->numberValue > 0)
          {
            vec[ix]->numberValue = upOp->numberValue;
          }
          else
          {
            vec[ix]->numberValue = 0;
          }
          delete vec[ix]->compoundValueP;
          vec[ix]->compoundValueP = NULL;
        }
        else if (op == "$mul")
        {
          vec[ix]->valueType = orion::ValueTypeNumber;
          vec[ix]->numberValue = 0;
          delete vec[ix]->compoundValueP;
          vec[ix]->compoundValueP = NULL;
        }
        else if ((op == "$push") || (op == "$addToSet"))
        {
          orion::CompoundValueNode* v = new orion::CompoundValueNode(orion::ValueTypeVector);
          orion::CompoundValueNode* inner;
          vec[ix]->valueType = orion::ValueTypeVector;

          switch (upOp->valueType)
          {
          case orion::ValueTypeString:
            v->add(orion::ValueTypeString, "", upOp->stringValue);
            break;

          case orion::ValueTypeNumber:
            v->add(orion::ValueTypeNumber, "", upOp->numberValue);
            break;

          case orion::ValueTypeBoolean:
            v->add(orion::ValueTypeBoolean, "", upOp->boolValue);
            break;

          case orion::ValueTypeNull:
            v->add(orion::ValueTypeNull, "", "");
            break;

          case orion::ValueTypeVector:
            inner = new orion::CompoundValueNode(orion::ValueTypeVector);
            for (unsigned int jx = 0; jx < upOp->childV.size(); jx++)
            {
              inner->add(upOp->childV[jx]->clone());
            }
            v->add(inner);
            break;

          case orion::ValueTypeObject:
            inner = new orion::CompoundValueNode(orion::ValueTypeObject);
            for (unsigned int jx = 0; jx < upOp->childV.size(); jx++)
            {
              inner->add(upOp->childV[jx]->clone());
            }
            v->add(inner);
            break;

          case orion::ValueTypeNotGiven:
            LM_E(("Runtime Error (value not given in compound value)"));
            break;

          default:
            LM_E(("Runtime Error (unknown attribute value type: %d on attribute %s)", upOp->valueType, vec[ix]->name.c_str()));
          }

          // Replace old compound value (with $push) with the new one ([])
          delete vec[ix]->compoundValueP;
          vec[ix]->compoundValueP = v;
        }
        else if (op == "$set")
        {
          orion::CompoundValueNode* o = new orion::CompoundValueNode(orion::ValueTypeObject);
          //orion::CompoundValueNode* inner;
          vec[ix]->valueType = orion::ValueTypeObject;

          switch (upOp->valueType)
          {
          case orion::ValueTypeString:
          case orion::ValueTypeNumber:
          case orion::ValueTypeBoolean:
          case orion::ValueTypeNull:
          case orion::ValueTypeVector:
            // Nothing to do in the case of inconsisent $set, e.g. {$set: 1}
            // FIXME P4: this could be detected at parsing stage and deal with it as Bad Input, but it is harder...
            break;

          case orion::ValueTypeObject:
            for (unsigned int jx = 0; jx < upOp->childV.size(); jx++)
            {
              o->add(upOp->childV[jx]->clone());
            }
            break;

          case orion::ValueTypeNotGiven:
            LM_E(("Runtime Error (value not given in compound value)"));
            break;

          default:
            LM_E(("Runtime Error (unknown attribute value type: %d on attribute %s)", upOp->valueType, vec[ix]->name.c_str()));
          }

          // Replace old compound value (with $push) with the new one ([])
          delete vec[ix]->compoundValueP;
          vec[ix]->compoundValueP = o;

        }
        else if ((op == "$unset") || (op == "$pull") || (op == "$pullAll"))
        {
          // FIXME P5: we are forcing shadowed semantics here. It could be a problem if the attribute used in the update
          // triggering the notification (eg. A: {$unset: 1}) is also in the notifications.attrs list (note that shadowed
          // was designed for built-in attributes, and they are included if splicitelly included in notifications.attr).
          // However, this case would be *very rare* and the alternative (to keep track of items in vec[] to be removed
          // at the end of 'for' processing) would be much more complex
          vec[ix]->shadowed = true;
          delete vec[ix]->compoundValueP;
          vec[ix]->compoundValueP = NULL;
        }
        else
        {
          LM_E(("Runtime Error (unknown operator: %s", op.c_str()));
        }
      }
    }
  }
}
