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
* Author: Fermín Galán
*
*/
#include <utility>
#include <map>
#include <string>
#include <vector>
#include <set>

#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"

#include "common/limits.h"
#include "common/globals.h"
#include "common/string.h"
#include "common/sem.h"
#include "common/statistics.h"
#include "common/errorMessages.h"
#include "common/defaultValues.h"
#include "common/RenderFormat.h"
#include "apiTypesV2/HttpInfo.h"
#include "alarmMgr/alarmMgr.h"

#include "orionTypes/OrionValueType.h"
#include "apiTypesV2/HttpInfo.h"

#include "mongoBackend/MongoCommonUpdate.h"
#include "mongoBackend/connectionOperations.h"
#include "mongoBackend/safeMongo.h"
#include "mongoBackend/dbConstants.h"
#include "mongoBackend/dbFieldEncoding.h"
#include "mongoBackend/MongoGlobal.h"
#include "mongoBackend/connectionOperations.h"
#include "mongoBackend/TriggeredSubscription.h"
#include "mongoBackend/location.h"

#include "cache/subCache.h"
#include "rest/StringFilter.h"
#include "ngsi/Scope.h"
#include "rest/uriParamNames.h"

using std::string;
using std::map;
using std::auto_ptr;
using namespace orion;
using namespace mongo;



/* ****************************************************************************
*
* isNotCustomMetadata -
*
* Check that the parameter is a not custom metadata, i.e. one metadata without
* an special semantic to be interpreted by the context broker itself
*
* FIXME P2: this function probably could be moved to another place "closer" to metadata
*/
static bool isNotCustomMetadata(std::string md)
{
  if (md != NGSI_MD_ID        &&
      md != NGSI_MD_LOCATION  &&
      md != NGSI_MD_CREDATE   &&
      md != NGSI_MD_MODDATE)
  {
    return false;
  }

  return true;
}

/* ****************************************************************************
*
* hasMetadata -
*
* Check if a metadata is included in a (request) ContextAttribute.
* FIXME P1: excellent candidate for a method for either
*           ContextAttribute or MetadataVector (or both)
*/
static bool hasMetadata(std::string name, std::string type, ContextAttribute* caP)
{
  for (unsigned int ix = 0; ix < caP->metadataVector.size() ; ++ix)
  {
    Metadata* md = caP->metadataVector[ix];

    if ((md->name == name))
    {
      return true;
    }
  }

  return false;
}


/* ****************************************************************************
*
* matchMetadata -
*
* Returns if two metadata elements have the same name and type, taking into account that
* the type field could not be present (it is optional in NGSI)
*/
static bool matchMetadata(BSONObj& md1, BSONObj& md2)
{

  // Metadata is identified by name, type is not part of identity any more
  return  getStringFieldF(md1, ENT_ATTRS_MD_NAME) == getStringFieldF(md2, ENT_ATTRS_MD_NAME);

}


/* ****************************************************************************
*
* equalMetadataValues -
*
*/
static bool equalMetadataValues(BSONObj& md1, BSONObj& md2)
{

  bool md1TypeExist = md1.hasField(ENT_ATTRS_MD_TYPE);
  bool md2TypeExist = md2.hasField(ENT_ATTRS_MD_TYPE);

  // If type exists in one metadata but not in the other, then the result is unequality
  if ((md1TypeExist && !md2TypeExist) || (!md1TypeExist && md2TypeExist))
  {
    return false;
  }

  // If type exists in both metadata elments, check if they are the same
  if (md1TypeExist && md2TypeExist)
  {
    if (getFieldF(md1, ENT_ATTRS_MD_TYPE).type() != getFieldF(md2, ENT_ATTRS_MD_TYPE).type())
    {
      return false;
    }
    switch (getFieldF(md1, ENT_ATTRS_MD_TYPE).type())
    {
      /* FIXME not yet, issue #1068 Support array and object in metadata value
      case Object:
        ...
        break;

       case Array:
        ...
        break;
      */

    case NumberDouble:
      if (getFieldF(md1, ENT_ATTRS_MD_TYPE).Number() != getFieldF(md2, ENT_ATTRS_MD_TYPE).Number())
      {
        return false;
      }
      break;

    case Bool:
      if (getBoolFieldF(md1, ENT_ATTRS_MD_TYPE) != getBoolFieldF(md2, ENT_ATTRS_MD_TYPE))
      {
        return false;
      }
      break;

    case String:
      if (getStringFieldF(md1, ENT_ATTRS_MD_TYPE) != getStringFieldF(md2, ENT_ATTRS_MD_TYPE))
      {
        return false;
      }
      break;

    case jstNULL:
      if (!getFieldF(md2, ENT_ATTRS_MD_TYPE).isNull())
      {
        return false;
      }
      break;

    default:
      LM_E(("Runtime Error (unknown JSON type for metadata NGSI type: %d)", getFieldF(md1, ENT_ATTRS_MD_TYPE).type()));
      return false;
      break;
    }
  }

  // declared types are equal. Same value ?
  if (getFieldF(md1, ENT_ATTRS_MD_VALUE).type() != getFieldF(md2, ENT_ATTRS_MD_VALUE).type())
  {
    return false;
  }

  switch (getFieldF(md1, ENT_ATTRS_MD_VALUE).type())
  {
    /* FIXME not yet
    case Object:
      ...
      break;

    case Array:
      ...
      break;
    */

    case NumberDouble:
      return getFieldF(md1, ENT_ATTRS_MD_VALUE).Number() == getFieldF(md2, ENT_ATTRS_MD_VALUE).Number();

    case Bool:
      return getBoolFieldF(md1, ENT_ATTRS_MD_VALUE) == getBoolFieldF(md2, ENT_ATTRS_MD_VALUE);

    case String:
      return getStringFieldF(md1, ENT_ATTRS_MD_VALUE) == getStringFieldF(md2, ENT_ATTRS_MD_VALUE);

    case jstNULL:
      return getFieldF(md2, ENT_ATTRS_MD_VALUE).isNull();

    default:
      LM_E(("Runtime Error (unknown metadata value type in DB: %d)", getFieldF(md1, ENT_ATTRS_MD_VALUE).type()));
      return false;
  }

}

/* ****************************************************************************
*
* equalMetadataVectors -
*
* Given two vectors with the same metadata names-types, check that all the values
* are equal, returning false otherwise.
*
* Arguments are passed by reference to avoid "heavy" copies
*/
static bool equalMetadataVectors(BSONObj& mdV1, BSONObj& mdV2)
{

  bool found = false;

  for (BSONObj::iterator i1 = mdV1.begin(); i1.more();)
  {
    BSONObj md1 = i1.next().embeddedObject();

    for (BSONObj::iterator i2 = mdV2.begin(); i2.more();)
    {
      BSONObj md2 = i2.next().embeddedObject();

      /* Check metadata match */
      if (matchMetadata(md1, md2))
      {
        if (!equalMetadataValues(md1, md2))
        {
          return false;
        }
        else
        {
          found = true;
          break;  // loop in i2
        }
      }
    }

    if (found)
    {
      /* Shortcut for early passing to the next attribute in i1 */
      found = false;
      continue;  // loop in i1
    }
  }

  return true;
}


/* ****************************************************************************
*
* attributeValueAbsent -
*
* Check that the attribute doesn't have any value
*
*/
bool attributeValueAbsent(ContextAttribute* caP, const string& apiVersion)
{
  /* In v2, absent attribute means "null", which has diferent semantics */
  return ((caP->valueType == ValueTypeNone) && (apiVersion == "v1"));
}

/* ****************************************************************************
*
* attributeTypeAbsent -
*
* Check that the attribute doesn't have any type
*
*/
bool attributeTypeAbsent(ContextAttribute* caP)
{
  // FIXME P10: this is a temporal solution while the ContextAttribute class gets
  // modified to inlcude a "NoneType" or similar. Type "" should be allowed in NGSIv2
  return caP->type == "";
}


/* ****************************************************************************
*
* changedAttr -
*/
bool attrValueChanges(BSONObj& attr, ContextAttribute* caP, std::string apiVersion)
{
  /* Not finding the attribute field at MongoDB is consideres as an implicit "" */
  if (!attr.hasField(ENT_ATTRS_VALUE))
  {
    return (caP->valueType != ValueTypeString || caP->stringValue != "");
  }

  /* No value in the request means that the value stays as it was before, so it is not
   * a change */
  if (caP->valueType == ValueTypeNone && apiVersion !="v2")
  {
    return false;
  }

  switch (getFieldF(attr, ENT_ATTRS_VALUE).type())
  {
    case Object:
    case Array:
      /* As the compoundValueP has been checked is NULL before invoking this function, finding
       * a compound value in DB means that there is a change */
      return true;

    case NumberDouble:
      return caP->valueType != ValueTypeNumber || caP->numberValue != getFieldF(attr, ENT_ATTRS_VALUE).Number();

    case Bool:
      return caP->valueType != ValueTypeBoolean || caP->boolValue != getBoolFieldF(attr, ENT_ATTRS_VALUE);

    case String:
      return caP->valueType != ValueTypeString || caP->stringValue != getStringFieldF(attr, ENT_ATTRS_VALUE);

    case jstNULL:
      return caP->valueType != ValueTypeNone;

    default:
      LM_E(("Runtime Error (unknown attribute value type in DB: %d)", getFieldF(attr, ENT_ATTRS_VALUE).type()));
      return false;
  }
}

/* ****************************************************************************
*
* appendMetadata -
*/
void appendMetadata(BSONArrayBuilder* mdVBuilder, const Metadata* mdP, bool useDefaultType)
{
  std::string type =  mdP->type;
  if (!mdP->typeGiven && useDefaultType)
  {
    type = DEFAULT_TYPE;
  }
  if (type != "")
  {

    switch (mdP->valueType)
    {
    case orion::ValueTypeString:
      mdVBuilder->append(BSON(ENT_ATTRS_MD_NAME << mdP->name << ENT_ATTRS_MD_TYPE << type << ENT_ATTRS_MD_VALUE << mdP->stringValue));
      return;

    case orion::ValueTypeNumber:
      mdVBuilder->append(BSON(ENT_ATTRS_MD_NAME << mdP->name << ENT_ATTRS_MD_TYPE << type << ENT_ATTRS_MD_VALUE << mdP->numberValue));
      return;

    case orion::ValueTypeBoolean:
      mdVBuilder->append(BSON(ENT_ATTRS_MD_NAME << mdP->name << ENT_ATTRS_MD_TYPE << type << ENT_ATTRS_MD_VALUE << mdP->boolValue));
      return;

    case orion::ValueTypeNone:
      mdVBuilder->append(BSON(ENT_ATTRS_MD_NAME << mdP->name << ENT_ATTRS_MD_TYPE << type << ENT_ATTRS_MD_VALUE << BSONNULL));
      return;

    default:
      LM_E(("Runtime Error (unknown metadata type: %d)", mdP->valueType));
    }
  }
  else
  {

    switch (mdP->valueType)
    {
    case orion::ValueTypeString:
      mdVBuilder->append(BSON(ENT_ATTRS_MD_NAME << mdP->name << ENT_ATTRS_MD_VALUE << mdP->stringValue));
      return;

    case orion::ValueTypeNumber:
      mdVBuilder->append(BSON(ENT_ATTRS_MD_NAME << mdP->name << ENT_ATTRS_MD_VALUE << mdP->numberValue));
      return;

    case orion::ValueTypeBoolean:
      mdVBuilder->append(BSON(ENT_ATTRS_MD_NAME << mdP->name << ENT_ATTRS_MD_VALUE << mdP->boolValue));
      return;

    case orion::ValueTypeNone:
      mdVBuilder->append(BSON(ENT_ATTRS_MD_NAME << mdP->name << ENT_ATTRS_MD_VALUE << BSONNULL));
      return;

    default:
      LM_E(("Runtime Error (unknown metadata type)"));
    }
  }
}

/* ****************************************************************************
*
* mergeAttrInfo -
*
* Takes as input the information of a given attribute, both in database (attr) and
* request (caP), and merged them producing the mergedAttr output. The function returns
* true if it was an actual update, false otherwise.
*/
static bool mergeAttrInfo(BSONObj& attr, ContextAttribute* caP, BSONObj* mergedAttr, const std::string& apiVersion)
{
  BSONObjBuilder ab;

  /* 1. Add value, if present in the request (it could be omitted in the case of updating only metadata).
   *    When the value of the attribute is empty (no update needed/wanted), then the value of the attribute is
   *    'copied' from DB to the variable 'ab' and sent back to mongo, to not destroy the value  */
  if (!attributeValueAbsent(caP, apiVersion))
  {
    caP->valueBson(ab);
  }
  else
  {
    /* Slightly different treatment, depending on attribute value type in DB (string, number, boolean, vector or object) */
    switch (getFieldF(attr, ENT_ATTRS_VALUE).type())
    {
      case Object:
        ab.append(ENT_ATTRS_VALUE, getFieldF(attr, ENT_ATTRS_VALUE).embeddedObject());
        break;

      case Array:
        ab.appendArray(ENT_ATTRS_VALUE, getFieldF(attr, ENT_ATTRS_VALUE).embeddedObject());
        break;

      case NumberDouble:
        ab.append(ENT_ATTRS_VALUE, getFieldF(attr, ENT_ATTRS_VALUE).Number());
        break;

      case Bool:
        ab.append(ENT_ATTRS_VALUE, getBoolFieldF(attr, ENT_ATTRS_VALUE));
        break;

      case String:
        ab.append(ENT_ATTRS_VALUE, getStringFieldF(attr, ENT_ATTRS_VALUE));
        break;

      case jstNULL:
        ab.appendNull(ENT_ATTRS_VALUE);
        break;

      default:
        LM_E(("Runtime Error (unknown attribute value type in DB: %d)", getFieldF(attr, ENT_ATTRS_VALUE).type()));
    }
  }

  /* 2. Add type, if present in request. If not, just use the one that is already present in the database. */
  if (caP->type != "")
  {
    ab.append(ENT_ATTRS_TYPE, caP->type);
  }
  else
  {
    if (attr.hasField(ENT_ATTRS_TYPE))
    {
      ab.append(ENT_ATTRS_TYPE, getStringFieldF(attr, ENT_ATTRS_TYPE));
    }
  }

  /* 3. Add metadata */
  BSONArrayBuilder mdVBuilder;

  /* First add the metadata elements coming in the request */
  for (unsigned int ix = 0; ix < caP->metadataVector.size() ; ++ix)
  {
    Metadata* mdP = caP->metadataVector[ix];

    /* Skip not custom metadata */
    if (isNotCustomMetadata(mdP->name))
    {
      continue;
    }

    appendMetadata(&mdVBuilder, mdP, apiVersion == "v2");
  }

  /* Second, for each metadata previously in the metadata vector but *not included in the request*, add it as is */
  int      mdVSize = 0;
  BSONObj  mdV;

  if (attr.hasField(ENT_ATTRS_MD))
  {
    mdV = getFieldF(attr, ENT_ATTRS_MD).embeddedObject();

    for (BSONObj::iterator i = mdV.begin(); i.more();)
    {
      Metadata  md(i.next().embeddedObject());

      mdVSize++;

      if (!hasMetadata(md.name, md.type, caP))
      {
        appendMetadata(&mdVBuilder, &md, apiVersion == "v2");
      }
    }
  }

  BSONObj mdNewV = mdVBuilder.arr();


  if (mdVBuilder.arrSize() > 0)
  {
    ab.appendArray(ENT_ATTRS_MD, mdNewV);
  }

  /* 4. Add creation date */
  if (attr.hasField(ENT_ATTRS_CREATION_DATE))
  {
    ab.append(ENT_ATTRS_CREATION_DATE, getIntFieldF(attr, ENT_ATTRS_CREATION_DATE));
  }

  /* It was an actual update? */
  bool actualUpdate;

  if (caP->compoundValueP == NULL)
  {
    /* In the case of simple value, we consider there is an actual change if one or more of the following are true:
     *
     * 1) the value of the attribute changed (see attrValueChanges for details)
     * 2) the type of the attribute changed (in this case, !attr.hasField(ENT_ATTRS_TYPE) is needed, as attribute
     *    type is optional according to NGSI and the attribute may not have that field in the BSON)
     * 3) the metadata changed (this is done checking if the size of the original and final metadata vectors is
     *    different and, if they are of the same size, checking if the vectors are not equal)
     */
      actualUpdate = (attrValueChanges(attr, caP, apiVersion) ||
                      ((caP->type != "") && (!attr.hasField(ENT_ATTRS_TYPE) ||
                                             getStringFieldF(attr, ENT_ATTRS_TYPE) != caP->type) ) ||
                      mdVBuilder.arrSize() != mdVSize || !equalMetadataVectors(mdV, mdNewV));
  }
  else
  {
      // FIXME P6: in the case of compound value, it's more difficult to know if an attribute
      // has really changed its value (many levels have to be traversed). Until we can develop the
      // matching logic, we consider actualUpdate always true.
      actualUpdate = true;
  }

  /* 5. Add modification date (actual change only if actual update) */
  if (actualUpdate)
  {
    ab.append(ENT_ATTRS_MODIFICATION_DATE, getCurrentTime());
  }
  else
  {
    /* The hasField() check is needed to preserve compatibility with entities that were created
     * in database by a CB instance previous to the support of creation and modification dates */
    if (attr.hasField(ENT_ATTRS_MODIFICATION_DATE))
    {
      ab.append(ENT_ATTRS_MODIFICATION_DATE, getIntFieldF(attr, ENT_ATTRS_MODIFICATION_DATE));
    }
  }

  *mergedAttr = ab.obj();

  return actualUpdate;
}


/* ****************************************************************************
*
* contextAttributeCustomMetadataToBson -
*
* Generates the BSON for metadata vector to be inserted in database for a given atribute.
* If there is no custom metadata, then it returns false (true otherwise).
*
*/
static bool contextAttributeCustomMetadataToBson(BSONObj& mdV, const ContextAttribute* ca, bool useDefaultType)
{
  BSONArrayBuilder  mdToAdd;

  for (unsigned int ix = 0; ix < ca->metadataVector.size(); ++ix)
  {
    const Metadata* md = ca->metadataVector[ix];

    if (!isNotCustomMetadata(md->name))
    {
      appendMetadata(&mdToAdd, md, useDefaultType);
      LM_T(LmtMongo, ("new custom metadata: {name: %s, type: %s, value: %s}",
                      md->name.c_str(), md->type.c_str(), md->toStringValue().c_str()));
    }
  }

  if (mdToAdd.arrSize() > 0)
  {
    mdV = mdToAdd.arr();
    return true;
  }

  return false;
}


/* ****************************************************************************
*
* updateAttribute -
*
* Returns true if an attribute was found, false otherwise. If true,
* the "actualUpdate" argument (passed by reference) is set to true in the case that the
* original value of the attribute was different than the one used in the update (this is
* important for ONCHANGE notifications)
*
* The isReplace boolean specifies how toSet has to be filled, either:
*
*   { attrs.A1: { ... }, attrs.A2: { ... } }  (in the case of isPeplace = false)
*
* or
*
*   { A1: { ... }, A2: { ... } }              (in the case of isPeplace = true)
*
* The former is to be used with { $set: <toSet> }, the later to be used with { attrs: <toSet> }
*
* In addition, in the case of isReplace, the attribute is added to toPush (otherwise, toPush is not
* touched).
*
*/
static bool updateAttribute
(
  BSONObj&            attrs,
  BSONObjBuilder*     toSet,
  BSONArrayBuilder*   toPush,
  ContextAttribute*   caP,
  bool&               actualUpdate,
  bool                isReplace,
  const string&       apiVersion
)
{
  actualUpdate = false;

  /* Attributes with metadata ID are stored as <attrName>__<ID> in the attributes embedded document */
  std::string effectiveName = dbDotEncode(caP->name);
  if (caP->getId() != "")
  {
    effectiveName += "__" + caP->getId();
  }

  if (isReplace)
  {
    actualUpdate = true;
    BSONObjBuilder newAttr;

    int now = getCurrentTime();

    if (!caP->typeGiven && (apiVersion == "v2"))
    {
      newAttr.append(ENT_ATTRS_TYPE, DEFAULT_TYPE);
    }
    else
    {
      newAttr.append(ENT_ATTRS_TYPE, caP->type);
    }
    newAttr.append(ENT_ATTRS_CREATION_DATE, now);
    newAttr.append(ENT_ATTRS_MODIFICATION_DATE, now);

    caP->valueBson(newAttr);

    /* Custom metadata */
    BSONObj mdV;
    if (contextAttributeCustomMetadataToBson(mdV, caP, apiVersion == "v2"));
    {
      newAttr.appendArray(ENT_ATTRS_MD, mdV);
    }

    toSet->append(effectiveName, newAttr.obj());
    toPush->append(caP->name);
  }
  else
  {
    if (!attrs.hasField(effectiveName.c_str()))
    {
      return false;
    }

    BSONObj newAttr;
    BSONObj attr = getFieldF(attrs, effectiveName).embeddedObject();
    actualUpdate = mergeAttrInfo(attr, caP, &newAttr, apiVersion);
    if (actualUpdate)
    {
      const std::string composedName = std::string(ENT_ATTRS) + "." + effectiveName;
      toSet->append(composedName, newAttr);
    }
  }

  return true;
}


/* ****************************************************************************
*
* appendAttribute -
*
* The "actualUpdate" argument (passed by reference) is set to true 1) in the case
* of actual append that, or 2) in the case of append as update if the
* original value of the attribute was different than the one used in the update (this is
* important for ONCHANGE notifications). Otherwise it is false
*
* In addition, return value is as follows:
* - true: there was an actual append change
* - false: there was an append-as-update change
*
* Attributes with metadata ID are stored as <attrName>_<ID> in the attributes embedded document
*/
static bool appendAttribute
(
  BSONObj&            attrs,
  BSONObjBuilder*     toSet,
  BSONArrayBuilder*   toPush,
  ContextAttribute*   caP,
  bool&               actualUpdate,
  const std::string&  apiVersion
)
{
  std::string effectiveName = dbDotEncode(caP->name);

  if (caP->getId() != "")
  {
    effectiveName += "__" + caP->getId();
  }

  /* APPEND with existing attribute equals to UPDATE */
  if (attrs.hasField(effectiveName.c_str()))
  {
    updateAttribute(attrs, toSet, toPush, caP, actualUpdate, false, apiVersion);
    return false;
  }

  /* Build the attribute to append */
  BSONObjBuilder ab;

  /* 1. Value */
  caP->valueBson(ab);

  /* 2. Type */
  if ((apiVersion == "v2") && !caP->typeGiven)
  {
    ab.append(ENT_ATTRS_TYPE, DEFAULT_TYPE);
  }
  else
  {
    ab.append(ENT_ATTRS_TYPE, caP->type);
  }

  /* 3. Metadata */
  BSONObj mdV;

  if (contextAttributeCustomMetadataToBson(mdV, caP, apiVersion == "v2"))
  {
      ab.appendArray(ENT_ATTRS_MD, mdV);
  }

  /* 4. Dates */
  int now = getCurrentTime();
  ab.append(ENT_ATTRS_CREATION_DATE, now);
  ab.append(ENT_ATTRS_MODIFICATION_DATE, now);

  const std::string composedName = std::string(ENT_ATTRS) + "." + effectiveName;
  toSet->append(composedName, ab.obj());
  toPush->append(caP->name);

  actualUpdate = true;
  return true;
}


/* ****************************************************************************
*
* legalIdUsage -
*
* Check that the client is not trying to mix attributes ID and no ID for the same
* name
*
*/
static bool legalIdUsage(BSONObj& attrs, ContextAttribute* caP)
{
  std::string prefix = caP->name + "__";

  if (caP->getId() == "")
  {
    /* Attribute attempting to append doesn't have any ID. Thus, no attribute with same name can have ID in attrs,
     * i.e. no attribute starting with "<attrName>" can at the same time start with "<attrName>__" */
    std::set<std::string> attrNames;
    attrs.getFieldNames(attrNames);

    for (std::set<std::string>::iterator i = attrNames.begin(); i != attrNames.end(); ++i)
    {
      std::string attrName = *i;

      if (strncmp(caP->name.c_str(), attrName.c_str(), caP->name.length()) == 0 &&
          strncmp(prefix.c_str(), attrName.c_str(), strlen(prefix.c_str())) == 0)
      {
        return false;
      }
    }
  }
  else
  {
    /* Attribute attempting to append has ID. Thus, no attribute with same name cannot have ID in attrs,
     * i.e. no attribute starting with "<attrName>" can at the same time have a name not starting with "<attrName>__"
     */
    std::set<std::string> attrNames;

    attrs.getFieldNames(attrNames);

    for (std::set<std::string>::iterator i = attrNames.begin(); i != attrNames.end(); ++i)
    {
      std::string attrName = *i;

      if (strncmp(caP->name.c_str(), attrName.c_str(), strlen(caP->name.c_str())) == 0 &&
          strncmp(prefix.c_str(), attrName.c_str(), strlen(prefix.c_str())) != 0)
      {
        return false;
      }
    }
  }

  return true;
}


/* ****************************************************************************
*
* legalIdUsage -
*
* Check that the client is not trying to mix attributes ID and no ID for the same
* name
*
*/
static bool legalIdUsage(const ContextAttributeVector& caV)
{
  for (unsigned int ix = 0; ix < caV.size(); ++ix)
  {
    std::string  attrName  = caV[ix]->name;
    std::string  attrType  = caV[ix]->type;
    std::string  attrId    = caV[ix]->getId();

    if (attrId == "")
    {
      /* Search for attribute with same name and type, but with actual ID to detect inconsistency */
      for (unsigned int jx = 0; jx < caV.size(); ++jx)
      {
        const ContextAttribute* ca = caV[jx];

        if (attrName == ca->name && attrType == ca->type && ca->getId() != "")
        {
          return false;
        }
      }
    }
  }

  return true;
}



/* ****************************************************************************
*
* deleteAttribute -
*
* Returns true if an attribute was deleted, false otherwise
*
* Attributes with metadata ID are stored as <attrName>__<ID> in the attributes embedded document
*/
static bool deleteAttribute
(
  BSONObj&                              attrs,
  BSONObjBuilder*                       toUnset,
  std::map<std::string, unsigned int>*  deleted,
  ContextAttribute*                     caP
)
{
  std::string effectiveName = dbDotEncode(caP->name);

  if (caP->getId() != "")
  {
    effectiveName += "__" + caP->getId();
  }

  if (!attrs.hasField(effectiveName.c_str()))
  {
    return false;
  }

  const std::string composedName = std::string(ENT_ATTRS) + "." + effectiveName;
  toUnset->append(composedName, 1);

  /* Record the ocurrence of this attribute. Only attributes using ID may have a value greater than 1 */
  if (deleted->count(caP->name))
  {
    std::map<std::string, unsigned int>::iterator it = deleted->find(caP->name);
    it->second++;
  }
  else
  {
    deleted->insert(std::make_pair(caP->name, 1));
  }

  return true;
}


/* ****************************************************************************
*
* servicePathSubscriptionRegex -
*
* 1. If the incoming request is without service path, then only subscriptions without
*    service path is a match (without or with '/#', or '/')
* 2. If the incoming request has a service path, then the REGEX must be created:
*    - Incoming: /a1/a2/a3
*    - REGEX: ^/#$ | ^/a1/#$ | | ^/a1/a2/#$ | ^/a1/a2/a3/#$ | ^/a1/a2/a3$
*
*/
std::string servicePathSubscriptionRegex(const std::string& servicePath, std::vector<std::string>& spathV)
{
  std::string  spathRegex;
  int          spathComponents = 0;


  //
  // Split Service Path in 'path components'
  //
  if (servicePath != "")
  {
    spathComponents = stringSplit(servicePath, '/', spathV);
  }

  if (spathComponents == 0)
  {
    spathRegex = "^$|^\\/#$|^\\/$";
  }
  else
  {
    //
    // 1. Empty or '/#'
    //
    spathRegex = std::string("^$|^\\/#$");



    //
    // 2. The whole list /a | /a/b | /a/b/c  etc
    //
    for (int ix = 0; ix < spathComponents; ++ix)
    {
      spathRegex += std::string("|^");

      for (int cIx = 0; cIx <= ix; ++cIx)
      {
        spathRegex += std::string("\\/") + spathV[cIx];
      }

      spathRegex += std::string("\\/#$");
    }


    //
    // 3. EXACT service path
    //
    spathRegex += std::string("|^");
    for (int cIx = 0; cIx < spathComponents; ++cIx)
    {
      spathRegex += std::string("\\/") + spathV[cIx];
    }
    spathRegex += std::string("$");
  }

  return spathRegex;
}


/* ****************************************************************************
*
* addTriggeredSubscriptions_withCache
*
*/
static bool addTriggeredSubscriptions_withCache
(
  std::string                               entityId,
  std::string                               entityType,
  const std::vector<std::string>&           modifiedAttrs,
  std::map<string, TriggeredSubscription*>& subs,
  std::string&                              err,
  std::string                               tenant,
  const std::vector<std::string>&           servicePathV
)
{  
  std::string                       servicePath     = (servicePathV.size() > 0)? servicePathV[0] : "";
  std::vector<CachedSubscription*>  subVec;

  cacheSemTake(__FUNCTION__, "match subs for notifications");
  subCacheMatch(tenant.c_str(), servicePath.c_str(), entityId.c_str(), entityType.c_str(), modifiedAttrs, &subVec);
  LM_T(LmtSubCache, ("%d subscriptions in cache match the update", subVec.size()));

  int now = getCurrentTime();
  for (unsigned int ix = 0; ix < subVec.size(); ++ix)
  {
    CachedSubscription* cSubP = subVec[ix];

    // Outdated subscriptions are skipped
    if (cSubP->expirationTime < now)
    {
      LM_T(LmtSubCache, ("%s is EXPIRED (EXP:%lu, NOW:%lu, DIFF: %d)", cSubP->subscriptionId, cSubP->expirationTime, now, now - cSubP->expirationTime));
      continue;
    }

    // Status is inactive
    if (cSubP->status == STATUS_INACTIVE)
    {
      LM_T(LmtSubCache, ("%s is INACTIVE", cSubP->subscriptionId));
      continue;
    }


    //
    // FIXME P4: See issue #2076.
    //           aList is just a copy of cSubP->attributes - would be good to avoid
    //           as a reference to the CachedSubscription is already in TriggeredSubscription
    //           cSubP->attributes is of type    std::vector<std::string>
    //           while AttributeList contains a  std::vector<std::string>
    //           Practically the same, except for the methods that AttributeList offers.
    //           Perhaps CachedSubscription should include an AttributeList (cSubP->attributes)
    //           instead of its std::vector<std::string> ... ?
    //
    AttributeList aList;

    aList.fill(cSubP->attributes);

    // Throttling
    if ((cSubP->throttling != -1) && (cSubP->lastNotificationTime != 0))
    {
      if ((now - cSubP->lastNotificationTime) < cSubP->throttling)
      {
        LM_T(LmtSubCache, ("subscription '%s' ignored due to throttling (T: %lu, LNT: %lu, NOW: %lu, NOW-LNT: %lu, T: %lu)",
                           cSubP->subscriptionId,
                           cSubP->throttling,
                           cSubP->lastNotificationTime,
                           now,
                           now - cSubP->lastNotificationTime,
                           cSubP->throttling));
        continue;
      }
      else
      {
        LM_T(LmtSubCache, ("subscription '%s' NOT ignored due to throttling (T: %lu, LNT: %lu, NOW: %lu, NOW-LNT: %lu, T: %lu)",
                           cSubP->subscriptionId,
                           cSubP->throttling,
                           cSubP->lastNotificationTime,
                           now,
                           now - cSubP->lastNotificationTime,
                           cSubP->throttling));
      }
    }
    else
    {
      LM_T(LmtSubCache, ("subscription '%s' NOT ignored due to throttling II (T: %lu, LNT: %lu, NOW: %lu, NOW-LNT: %lu, T: %lu)",
                         cSubP->subscriptionId,
                         cSubP->throttling,
                         cSubP->lastNotificationTime,
                         now,
                         now - cSubP->lastNotificationTime,
                         cSubP->throttling));
    }

    TriggeredSubscription* subP = new TriggeredSubscription((long long) cSubP->throttling,
                                                           (long long) cSubP->lastNotificationTime,
                                                           cSubP->renderFormat,
                                                           cSubP->httpInfo,
                                                           aList,
                                                           cSubP->subscriptionId,
                                                           cSubP->tenant);
    subP->blacklist = cSubP->blacklist;

    subP->fillExpression(cSubP->expression.geometry, cSubP->expression.coords, cSubP->expression.georel);

    std::string errorString;

    if (!subP->stringFilterSet(&cSubP->expression.stringFilter, &errorString))
    {
      LM_E(("Runtime Error (error setting string filter: %s)", errorString.c_str()));
      delete subP;
      cacheSemGive(__FUNCTION__, "match subs for notifications");
      return false;
    }

    subs.insert(std::pair<string, TriggeredSubscription*>(cSubP->subscriptionId, subP));
  }

  cacheSemGive(__FUNCTION__, "match subs for notifications");
  return true;
}



/* ****************************************************************************
*
* addTriggeredSubscriptions_noCache
*
*/
static bool addTriggeredSubscriptions_noCache
(
  std::string                               entityId,
  std::string                               entityType,
  const std::vector<std::string>&           modifiedAttrs,
  std::map<string, TriggeredSubscription*>& subs,
  std::string&                              err,
  std::string                               tenant,
  const std::vector<std::string>&           servicePathV
)
{
  std::string               servicePath     = (servicePathV.size() > 0)? servicePathV[0] : "";
  std::vector<std::string>  spathV;
  std::string               spathRegex      = servicePathSubscriptionRegex(servicePath, spathV);


  //
  // Create the REGEX for the Service Path
  //
  spathRegex = std::string("/") + spathRegex + "/";


  /* Build query */
  std::string entIdQ       = CSUB_ENTITIES   "." CSUB_ENTITY_ID;
  std::string entTypeQ     = CSUB_ENTITIES   "." CSUB_ENTITY_TYPE;
  std::string entPatternQ  = CSUB_ENTITIES   "." CSUB_ENTITY_ISPATTERN;
  std::string condTypeQ    = CSUB_CONDITIONS "." CSUB_CONDITIONS_TYPE;
  std::string inRegex      = "{ $in: [ " + spathRegex + ", null ] }";
  BSONObj     spBson       = fromjson(inRegex);

  /* Note the $or on entityType, to take into account matching in subscriptions with no entity type */
  BSONObj queryNoPattern = BSON(
                entIdQ << entityId <<
                "$or" << BSON_ARRAY(
                    BSON(entTypeQ << entityType) <<
                    BSON(entTypeQ << BSON("$exists" << false))) <<
                entPatternQ << "false" <<
                condTypeQ << ON_CHANGE_CONDITION <<
                CSUB_EXPIRATION   << BSON("$gt" << (long long) getCurrentTime()) <<
                CSUB_STATUS << BSON("$ne" << STATUS_INACTIVE) <<
                CSUB_SERVICE_PATH << spBson);

  /* This is JavaScript code that runs in MongoDB engine. As far as I know, this is the only
   * way to do a "reverse regex" query in MongoDB (see
   * http://stackoverflow.com/questions/15966991/mongodb-reverse-regex/15989520).
   * Note that although we are using a isPattern=true in the MongoDB query besides $where, we
   * also need to check that in the if statement in the JavaScript function given that a given
   * sub document could include both isPattern=true and isPattern=false documents */
  std::string function = std::string("function()") +
         "{" +
            "for (var i=0; i < this."+CSUB_ENTITIES+".length; i++) {" +
                "if (this."+CSUB_ENTITIES+"[i]."+CSUB_ENTITY_ISPATTERN+" == \"true\" && " +
                    "(this."+CSUB_ENTITIES+"[i]."+CSUB_ENTITY_TYPE+" == \""+entityType+"\" || " +
                        "this."+CSUB_ENTITIES+"[i]."+CSUB_ENTITY_TYPE+" == \"\" || " +
                        "!(\""+CSUB_ENTITY_TYPE+"\" in this."+CSUB_ENTITIES+"[i])) && " +
                    "\""+entityId+"\".match(this."+CSUB_ENTITIES+"[i]."+CSUB_ENTITY_ID+")) {" +
                    "return true; " +
                "}" +
            "}" +
            "return false; " +
         "}";
  LM_T(LmtMongo, ("JS function: %s", function.c_str()));

  BSONObjBuilder  queryPattern;

  queryPattern.append(entPatternQ, "true");
  queryPattern.append(condTypeQ, ON_CHANGE_CONDITION);
  queryPattern.append(CSUB_EXPIRATION, BSON("$gt" << (long long) getCurrentTime()));
  queryPattern.append(CSUB_STATUS, BSON("$ne" << STATUS_INACTIVE));
  queryPattern.append(CSUB_SERVICE_PATH, spBson);
  queryPattern.appendCode("$where", function);

  // FIXME: condTypeQ, condValueQ and servicePath part could be "factorized" out of the $or clause
  BSONObj                   query       = BSON("$or" << BSON_ARRAY(queryNoPattern << queryPattern.obj()));
  std::string               collection  = getSubscribeContextCollectionName(tenant);
  auto_ptr<DBClientCursor>  cursor;
  std::string               errorString;

  LM_T(LmtMongo, ("query() in '%s' collection: '%s'",
                  getSubscribeContextCollectionName(tenant).c_str(),
                  query.toString().c_str()));

  TIME_STAT_MONGO_READ_WAIT_START();
  DBClientBase* connection = getMongoConnection();
  if (collectionQuery(connection, collection, query, &cursor, &errorString) != true)
  {
    TIME_STAT_MONGO_READ_WAIT_STOP();
    releaseMongoConnection(connection);
    return false;
  }
  TIME_STAT_MONGO_READ_WAIT_STOP();

  /* For each one of the subscriptions found, add it to the map (if not already there),
   * after checking triggering attributes */
  while (moreSafe(cursor))
  {
    BSONObj     sub;
    std::string err;
    if (!nextSafeOrErrorF(cursor, &sub, &err))
    {
      LM_E(("Runtime Error (exception in nextSafe(): %s - query: %s)", err.c_str(), query.toString().c_str()));
      continue;
    }
    BSONElement  idField  = getFieldF(sub, "_id");

    //
    // BSONElement::eoo returns true if 'not found', i.e. the field "_id" doesn't exist in 'sub'
    //
    // Now, if 'getFieldF(sub, "_id")' is not found, if we continue, calling OID() on it, then we get
    // an exception and the broker crashes.
    //
    if (idField.eoo() == true)
    {
      std::string details = std::string("error retrieving _id field in doc: '") + sub.toString() + "'";
      alarmMgr.dbError(details);
      continue;
    }

    std::string subIdStr = idField.OID().toString();

    if (subs.count(subIdStr) == 0)
    {

      /* Except in the case of ONANYCHANGE subscriptions (the ones with empty condValues), we check if
       * condValues include some of the modifiedAttributes. In previous versions we defered this to DB
       * as an additional element in the csubs query (in both pattern and no-pattern "$or branches"), eg:
       *
       * "conditions.value": { $in: [ "pressure" ] }
       *
       * However, it is difficult to check this condition *OR* empty array (for the case of ONANYCHANGE)
       * at query level, so now do the check in the code.
       */
      if (!someEmptyCondValue(sub))
      {
        if (!condValueAttrMatch(sub, modifiedAttrs))
        {
          continue;
        }
      }

      LM_T(LmtMongo, ("adding subscription: '%s'", sub.toString().c_str()));

      long long           throttling         = sub.hasField(CSUB_THROTTLING)?       getIntOrLongFieldAsLongF(sub, CSUB_THROTTLING)       : -1;
      long long           lastNotification   = sub.hasField(CSUB_LASTNOTIFICATION)? getIntOrLongFieldAsLongF(sub, CSUB_LASTNOTIFICATION) : -1;
      std::string         renderFormatString = sub.hasField(CSUB_FORMAT)? getStringFieldF(sub, CSUB_FORMAT) : "";
      RenderFormat        renderFormat       = stringToRenderFormat(renderFormatString);
      ngsiv2::HttpInfo    httpInfo;

      httpInfo.fill(sub);

      TriggeredSubscription* trigs = new TriggeredSubscription
        (
          throttling,
          lastNotification,
          renderFormat,
          httpInfo,
          subToAttributeList(sub), "", "");

      trigs->blacklist = sub.hasField(CSUB_BLACKLIST)? getBoolFieldF(sub, CSUB_BLACKLIST) : false;

      if (sub.hasField(CSUB_EXPR))
      {
        BSONObj expr = getObjectFieldF(sub, CSUB_EXPR);

        std::string q        = expr.hasField(CSUB_EXPR_Q)      ? getStringFieldF(expr, CSUB_EXPR_Q)      : "";
        std::string georel   = expr.hasField(CSUB_EXPR_GEOREL) ? getStringFieldF(expr, CSUB_EXPR_GEOREL) : "";
        std::string geometry = expr.hasField(CSUB_EXPR_GEOM)   ? getStringFieldF(expr, CSUB_EXPR_GEOM)   : "";
        std::string coords   = expr.hasField(CSUB_EXPR_COORDS) ? getStringFieldF(expr, CSUB_EXPR_COORDS) : "";

        trigs->fillExpression(georel, geometry, coords);

        // Parsing q
        if (q != "")
        {
          StringFilter* stringFilterP = new StringFilter();

          if (stringFilterP->parse(q.c_str(), &err) == false)
          {
            delete stringFilterP;
          
            LM_E(("Runtime Error (%s)", err.c_str()));
            releaseMongoConnection(connection);
            return false;
          }
          else
          {
            std::string errorString;

            if (!trigs->stringFilterSet(stringFilterP, &errorString))
            {
              delete stringFilterP;

              LM_E(("Runtime Error (error setting string filter: %s)", errorString.c_str()));
              releaseMongoConnection(connection);
              return false;
            }

            delete stringFilterP;
          }
        }
      }

      subs.insert(std::pair<string, TriggeredSubscription*>(subIdStr, trigs));
    }
  }
  releaseMongoConnection(connection);

  return true;
}



/* ****************************************************************************
*
* addTriggeredSubscriptions - 
*
*/
static bool addTriggeredSubscriptions
(
  std::string                               entityId,
  std::string                               entityType,
  const std::vector<std::string>&           modifiedAttrs,
  std::map<string, TriggeredSubscription*>& subs,
  std::string&                              err,
  std::string                               tenant,
  const std::vector<std::string>&           servicePathV
)
{
  extern bool noCache;

  if (noCache)
  {
    return addTriggeredSubscriptions_noCache(entityId, entityType, modifiedAttrs, subs, err, tenant, servicePathV);
  }
  else
  {
    return addTriggeredSubscriptions_withCache(entityId, entityType, modifiedAttrs, subs, err, tenant, servicePathV);
  }
}

/* ****************************************************************************
*
* processOnChangeConditionForUpdateContext -
*
* This method returns true if the notification was actually sent. Otherwise, false
* is returned. This is used in the caller to know if lastNotification field in the
* subscription document in csubs collection has to be modified or not.
*/
static bool processOnChangeConditionForUpdateContext
(
  ContextElementResponse*          notifyCerP,
  const AttributeList&             attrL,
  std::string                      subId,
  RenderFormat                     renderFormat,
  std::string                      tenant,
  const std::string&               xauthToken,
  const std::string&               fiwareCorrelator,
  const std::vector<std::string>&  attrsOrder,
  const ngsiv2::HttpInfo&          httpInfo,
  bool                             blacklist = false
)
{
  NotifyContextRequest   ncr;
  ContextElementResponse cer;
  cer.contextElement.entityId.fill(&notifyCerP->contextElement.entityId);

  /* Fill NotifyContextRequest with cerP, filtering by attrL */
  for (unsigned int ix = 0; ix < notifyCerP->contextElement.contextAttributeVector.size(); ix++)
  {
    ContextAttribute* caP = notifyCerP->contextElement.contextAttributeVector[ix];

    if ((attrL.size() == 0) || (blacklist == true))
    {
      /* Empty attribute list in the subscription mean that all attributes are added */
      cer.contextElement.contextAttributeVector.push_back(caP);
    }
    else
    {
      for (unsigned int jx = 0; jx < attrL.size(); jx++)
      {
        /* 'skip' field is used to mark deleted attributes that must not be included in the
         * notification (see deleteAttrInNotifyCer function for details) */
        if (caP->name == attrL[jx] && !caP->skip)
        {
          cer.contextElement.contextAttributeVector.push_back(caP);
        }
      }
    }
  }

  /* Early exit without sending notification if attribute list is empty */
  if (cer.contextElement.contextAttributeVector.size() == 0)
  {
    ncr.contextElementResponseVector.release();
    return false;
  }

  /* Setting status code in CER */
  cer.statusCode.fill(SccOk);

  ncr.contextElementResponseVector.push_back(&cer);

  /* Complete the fields in NotifyContextRequest */
  ncr.subscriptionId.set(subId);
  // FIXME: we use a proper origin name
  ncr.originator.set("localhost");

  getNotifier()->sendNotifyContextRequest(&ncr,
                                          httpInfo,
                                          tenant,
                                          xauthToken,
                                          fiwareCorrelator,
                                          renderFormat,
                                          attrsOrder,
                                          blacklist);
  return true;
}



/* ****************************************************************************
*
* processSubscriptions - send a notification for each subscription in the map
*/
static bool processSubscriptions
(
  std::map<string, TriggeredSubscription*>& subs,
  ContextElementResponse*                   notifyCerP,
  std::string*                              err,
  const std::string&                        tenant,
  const std::string&                        xauthToken,
  const std::string&                        fiwareCorrelator
)
{
  bool ret = true;

  *err = "";

  for (std::map<string, TriggeredSubscription*>::iterator it = subs.begin(); it != subs.end(); ++it)
  {
    std::string             mapSubId  = it->first;
    TriggeredSubscription*  tSubP     = it->second;


    /* There are some checks to perform on TriggeredSubscription in order to see if the notification has to be actually sent. Note
     * that checks are done in increasing cost order (e.g. georel check is done at the end).
     *
     * Note that check for triggering based on attributes it isn't part of these checks: it has been already done
     * before adding the subscription to the map.
     */

    /* Check 1: timming (not expired and ok from throttling point of view) */
    if (tSubP->throttling != 1 && tSubP->lastNotification != 1)
    {
      long long current = getCurrentTime();
      long long sinceLastNotification = current - tSubP->lastNotification;

      if (tSubP->throttling > sinceLastNotification)
      {
        LM_T(LmtMongo, ("blocked due to throttling, current time is: %l", current));
        LM_T(LmtSubCache, ("ignored '%s' due to throttling, current time is: %l", tSubP->cacheSubId.c_str(), current));

        continue;
      }
    }

    /* Check 2: String Filter */
    if ((tSubP->stringFilterP != NULL) && (!tSubP->stringFilterP->match(notifyCerP)))
    {
      continue;
    }

    /* Check 3: expression (georel, which also uses geometry and coords) */
    // TBD (issue #1678)

    /* Send notification */
    LM_T(LmtSubCache, ("NOT ignored: %s", tSubP->cacheSubId.c_str()));

    bool  notificationSent;

    notificationSent = processOnChangeConditionForUpdateContext(notifyCerP,
                                                                tSubP->attrL,
                                                                mapSubId,
                                                                tSubP->renderFormat,
                                                                tenant,
                                                                xauthToken,
                                                                fiwareCorrelator,
                                                                tSubP->attrL.attributeV,
                                                                tSubP->httpInfo,
                                                                tSubP->blacklist);

    if (notificationSent)
    {
      long long rightNow = getCurrentTime();

      //
      // If broker running without subscription cache, put lastNotificationTime and count in DB 
      //
      if (subCacheActive == false)
      {
        BSONObj query  = BSON("_id" << OID(mapSubId));
        BSONObj update = BSON("$set" <<
                              BSON(CSUB_LASTNOTIFICATION << rightNow) <<
                              "$inc" << BSON(CSUB_COUNT << (long long) 1));
        
        ret = collectionUpdate(getSubscribeContextCollectionName(tenant), query, update, false, err);
      }


      //
      // Saving lastNotificationTime and count for cached subscription
      //
      if (tSubP->cacheSubId != "")
      {
        cacheSemTake(__FUNCTION__, "update lastNotificationTime for cached subscription");

        CachedSubscription*  cSubP = subCacheItemLookup(tSubP->tenant.c_str(), tSubP->cacheSubId.c_str());

        if (cSubP != NULL)
        {
          cSubP->lastNotificationTime = rightNow;
          cSubP->count               += 1;

          LM_T(LmtSubCache, ("set lastNotificationTime to %lu and count to %lu for '%s'", cSubP->lastNotificationTime, cSubP->count, cSubP->subscriptionId));
        }
        else
        {
          LM_E(("Runtime Error (cached subscription '%s' for tenant '%s' not found)",
                tSubP->cacheSubId.c_str(), tSubP->tenant.c_str()));
        }

        cacheSemGive(__FUNCTION__, "update lastNotificationTime for cached subscription");
      }
    }
  }

  releaseTriggeredSubscriptions(subs);

  return ret;
}



/* ****************************************************************************
*
* buildGeneralErrorResponse -
*/
static void buildGeneralErrorResponse
(
  ContextElement*         ceP,
  ContextAttribute*       caP,
  UpdateContextResponse*  responseP,
  HttpStatusCode          code,
  std::string             details = "",
  ContextAttributeVector* cavP    = NULL
)
{
  ContextElementResponse* cerP = new ContextElementResponse();
  cerP->contextElement.entityId = ceP->entityId;

  if (caP != NULL)
  {
    cerP->contextElement.contextAttributeVector.push_back(caP);
  }
  else if (cavP != NULL)
  {
    cerP->contextElement.contextAttributeVector.fill(cavP);
  }

  cerP->statusCode.fill(code, details);
  responseP->contextElementResponseVector.push_back(cerP);
}



/* ****************************************************************************
*
* setResponseMetadata -
*
* Common method to create the metadata elements in updateContext responses
*
*/
static void setResponseMetadata(ContextAttribute* caReq, ContextAttribute* caRes)
{
  Metadata*  md;

  /* Not custom */
  if (caReq->getId().length() > 0)
  {
    md = new Metadata(NGSI_MD_ID, "string", caReq->getId());
    caRes->metadataVector.push_back(md);
  }

  if ((caReq->getLocation().length() > 0) && (caReq->type != GEO_POINT))
  {
    /* Note that if attribute type is geo:point then the user is using the "new way"
     * of locating entities in NGSIv1, thus location metadata is not rendered */
    md = new Metadata(NGSI_MD_LOCATION, "string", caReq->getLocation());
    caRes->metadataVector.push_back(md);
  }

  /* Custom (just "mirroring" in the response) */
  for (unsigned int ix = 0; ix < caReq->metadataVector.size(); ++ix)
  {
    Metadata* mdReq = caReq->metadataVector[ix];

    if (!isNotCustomMetadata(mdReq->name))
    {
      md = new Metadata(mdReq);
      caRes->metadataVector.push_back(md);
    }
  }
}


/* ****************************************************************************
*
* howManyAttrs -
*
* Returns the number of occurences of the attrName attribute (e.g. "A1") in the attrs
* BSON (taken from DB). This use to be 1, except in the case of using ID.
*
*/
static unsigned int howManyAttrs(BSONObj& attrs, std::string& attrName)
{
  unsigned int          c = 0;
  std::set<std::string> attrNames;

  attrs.getFieldNames(attrNames);
  for (std::set<std::string>::iterator i = attrNames.begin(); i != attrNames.end(); ++i)
  {
    if (basePart(*i) == attrName)
    {
      c++;
    }
  }

  return c;
}

/* ****************************************************************************
*
* updateAttrInNotifyCer -
*
*/
static void updateAttrInNotifyCer
(
  ContextElementResponse* notifyCerP,
  ContextAttribute*       targetAttr,
  bool                    useDefaultType
)
{
  /* Try to find the attribute in the notification CER */
  for (unsigned int ix = 0; ix < notifyCerP->contextElement.contextAttributeVector.size(); ix++)
  {
    ContextAttribute* caP = notifyCerP->contextElement.contextAttributeVector[ix];

    if (caP->name == targetAttr->name)
    {
      if (targetAttr->valueType != ValueTypeNone)
      {
        caP->valueType      = targetAttr->valueType;
        caP->stringValue    = targetAttr->stringValue;
        caP->boolValue      = targetAttr->boolValue;
        caP->numberValue    = targetAttr->numberValue;

        // Free memory used by the all compound value (if any)
        if (caP->compoundValueP != NULL)
        {
          delete caP->compoundValueP;
        }
        caP->compoundValueP = targetAttr->compoundValueP == NULL ? NULL : targetAttr->compoundValueP->clone();
      }
      if (targetAttr->type != "")
      {
        caP->type = targetAttr->type;
      }

      /* Metadata */
      for (unsigned int jx = 0; jx < targetAttr->metadataVector.size(); jx++)
      {
        Metadata* targetMdP = targetAttr->metadataVector[jx];
        /* Search for matching medatat in the CER attribute */
        bool matchMd = false;
        for (unsigned int kx = 0; kx < caP->metadataVector.size(); kx++)
        {
          Metadata* mdP = caP->metadataVector[kx];
          if (mdP->name == targetMdP->name)
          {
            mdP->valueType   = targetMdP->valueType;
            mdP->stringValue = targetMdP->stringValue;
            mdP->boolValue   = targetMdP->boolValue;
            mdP->numberValue = targetMdP->numberValue;
            if (targetMdP->type != "")
            {
              mdP->type = targetMdP->type;
            }
            matchMd = true;
            break;   /* kx  loop */
          }
        }

        /* If the attribute in target attr was not found, then it has to be added*/
        if (!matchMd)
        {
          Metadata* newMdP = new Metadata(targetMdP, useDefaultType);
          caP->metadataVector.push_back(newMdP);
        }
      }

      return;
    }
  }

  /* Reached this point, it means that it is a new attribute (APPEND case) */
  ContextAttribute* caP = new ContextAttribute(targetAttr, useDefaultType);

  if (caP->compoundValueP)
  {
    // The ContextAttribute constructor steals the compound, but in this case, it must be cloned
    targetAttr->compoundValueP = caP->compoundValueP->clone();
  }

  notifyCerP->contextElement.contextAttributeVector.push_back(caP);
}

/* ****************************************************************************
*
* deleteAttrInNotifyCer -
*
* The deletion algorithm is based on using the 'skip' flag in CA in order to
* mark attributes that must not be render in the notificationMode
*/
static void deleteAttrInNotifyCer
(
  ContextElementResponse* notifyCerP,
  ContextAttribute*       targetAttr
)
{  
  for (unsigned int ix = 0; ix < notifyCerP->contextElement.contextAttributeVector.size(); ix++)
  {
    ContextAttribute* caP = notifyCerP->contextElement.contextAttributeVector[ix];
    if (caP->name == targetAttr->name)
    {
      caP->skip = true;
    }
  }
}

/* ****************************************************************************
*
* updateContextAttributeItem -
*
*/
static bool updateContextAttributeItem
(
  ContextElementResponse*   cerP,
  ContextAttribute*         ca,
  BSONObj&                  attrs,
  ContextAttribute*         targetAttr,
  ContextElementResponse*   notifyCerP,
  EntityId*                 eP,
  BSONObjBuilder*           toSet,
  BSONArrayBuilder*         toPush,
  bool&                     actualUpdate,
  bool&                     entityModified,
  std::string*              currentLocAttrName,
  BSONObjBuilder*           geoJson,
  bool                      isReplace,
  const std::string&        apiVersion,
  OrionError*               oe
)
{
  std::string err;

  if (updateAttribute(attrs, toSet, toPush, targetAttr, actualUpdate, isReplace, apiVersion))
  {
    // Attribute was found
    entityModified = actualUpdate || entityModified;    
  }
  else
  {
    // Attribute was not found
    if (!isReplace)
    {
      /* If updateAttribute() returns false, then that particular attribute has not
       * been found. In this case, we interrupt the processing and early return with
       * an error StatusCode */
      // FIXME P10: not sure if this .fill() is useless... it seems it is "overriden" by
      // another .fill() in this function caller. We keep it by the moment, but it probably
      // will removed when we refactor this function
      std::string details = std::string("action: UPDATE") +
                            " - entity: [" + eP->toString() + "]" +
                            " - offending attribute: " + targetAttr->getName();
      cerP->statusCode.fill(SccInvalidParameter, details);
      oe->fill(SccContextElementNotFound, "No context element found", "NotFound");

      /* Although ca has been already pushed into cerP, it can be used */
      ca->found = false;
    }
  }

  /* Check aspects related with location */
  if (!processLocationAtUpdateAttribute(currentLocAttrName, targetAttr, geoJson, &err, apiVersion, oe))
  {
    std::string details = std::string("action: UPDATE") +
                          " - entity: [" + eP->toString() + "]" +
                          " - offending attribute: " + targetAttr->getName() +
                          " - " + err;
    cerP->statusCode.fill(SccInvalidParameter, details);
    // oe->fill() not used, as this is done internally in processLocationAtUpdateAttribute()

    alarmMgr.badInput(clientIp, err);
    return false;
  }

  updateAttrInNotifyCer(notifyCerP, targetAttr, apiVersion == "v2");

  return true;
}



/* ****************************************************************************
*
* appendContextAttributeItem -
*
*/
static bool appendContextAttributeItem
(
  ContextElementResponse*   cerP,
  BSONObj&                  attrs,
  ContextAttribute*         targetAttr,
  ContextElementResponse*   notifyCerP,
  EntityId*                 eP,
  BSONObjBuilder*           toSet,
  BSONArrayBuilder*         toPush,
  bool&                     actualUpdate,
  bool&                     entityModified,
  std::string*              currentLocAttrName,
  BSONObjBuilder*           geoJson,
  const std::string&        apiVersion,
  OrionError*               oe
)
{
  std::string err;

  if (!legalIdUsage(attrs, targetAttr))
  {
    /* If legalIdUsage() returns false, then that particular attribute can not be appended. In this case,
     * we interrupt the processing and early return with
     * a error StatusCode */
    std::string details = std::string("action: APPEND") +
                          " - entity: [" + eP->toString() + "]" +
                          " - offending attribute: " + targetAttr->getName() +
                          " - attribute cannot be appended";
    cerP->statusCode.fill(SccInvalidParameter, details);
    oe->fill(SccInvalidModification, details, "Unprocessable");

    alarmMgr.badInput(clientIp, "attribute cannot be appended");
    return false;
  }

  bool actualAppend = appendAttribute(attrs, toSet, toPush, targetAttr, actualUpdate, apiVersion);
  entityModified = actualUpdate || entityModified;

  /* Check aspects related with location */
  if (!processLocationAtAppendAttribute(currentLocAttrName, targetAttr, actualAppend, geoJson,
                                        &err, apiVersion, oe))
  {
    std::string details = std::string("action: APPEND") +
                          " - entity: [" + eP->toString() + "]" +
                          " - offending attribute: " + targetAttr->getName() +
                          " - " + err;
    cerP->statusCode.fill(SccInvalidParameter, details);
    // oe->fill() is not used here as it is managed by processLocationAtAppendAttribute()

    alarmMgr.badInput(clientIp, err);
    return false;
  }

  // Note that updateAttrInNotifyCer() may "ruin" targetAttr, as compoundValueP is moved
  // (not copied) to the structure in the notifyCerP and null-ified in targetAttr. Thus, it has
  // to be called after the location processing logic (as this logic may need the compoundValueP
  updateAttrInNotifyCer(notifyCerP, targetAttr, apiVersion == "v2");

  return true;
}

/* ****************************************************************************
*
* deleteContextAttributeItem -
*
*/
static bool deleteContextAttributeItem
(
  ContextElementResponse*               cerP,
  ContextAttribute*                     ca,
  BSONObj&                              attrs,
  ContextAttribute*                     targetAttr,
  ContextElementResponse*               notifyCerP,
  EntityId*                             eP,
  BSONObjBuilder*                       toUnset,
  bool&                                 entityModified,
  std::string*                          currentLocAttrName,
  std::map<std::string, unsigned int>*  deletedAttributesCounter,
  const std::string&                    apiVersion,
  OrionError*                           oe
)
{
  if (deleteAttribute(attrs, toUnset, deletedAttributesCounter, targetAttr))
  {
    deleteAttrInNotifyCer(notifyCerP, targetAttr);
    entityModified = true;

    /* Check aspects related with location */
    if (targetAttr->getLocation(apiVersion).length() > 0)
    {
      std::string details = std::string("action: DELETE") +
                            " - entity: [" + eP->toString() + "]" +
                            " - offending attribute: " + targetAttr->getName() +
                            " - location attribute has to be defined at creation time, with APPEND";
      cerP->statusCode.fill(SccInvalidParameter, details);
      oe->fill(SccInvalidModification, details, "Unprocessable");

      alarmMgr.badInput(clientIp, "location attribute has to be defined at creation time");
      return false;
    }

    /* Check aspects related with location. "Nullining" currentLocAttrName is the way of specifying
     * that location field is no longer used */
    if (*currentLocAttrName == targetAttr->name)
    {
      *currentLocAttrName = "";
    }

    ca->found = true;
  }
  else
  {
    /* If deleteAttribute() returns false, then that particular attribute has not
     * been found. In this case, we interrupt the processing and early return with
     * a error StatusCode */
    std::string details = std::string("action: DELETE") +
                          " - entity: [" + eP->toString() + "]" +
                          " - offending attribute: " + targetAttr->getName() +
                          " - attribute not found";
    cerP->statusCode.fill(SccInvalidParameter, details);
    oe->fill(SccContextElementNotFound, "Attribute not found", "NotFound");

    alarmMgr.badInput(clientIp, "attribute to be deleted is not found");
    ca->found = false;

    return false;
  }

  return true;
}


/* ****************************************************************************
*
* processContextAttributeVector -
*
* Returns true if entity was actually modified, false otherwise (including fail cases)
*
*/
static bool processContextAttributeVector
(
  ContextElement*                            ceP,
  std::string                                action,
  std::map<string, TriggeredSubscription*>&  subsToNotify,
  ContextElementResponse*                    notifyCerP,
  BSONObj&                                   attrs,
  BSONObjBuilder*                            toSet,
  BSONObjBuilder*                            toUnset,
  BSONArrayBuilder*                          toPush,
  BSONArrayBuilder*                          toPull,
  ContextElementResponse*                    cerP,
  std::string*                               currentLocAttrName,
  BSONObjBuilder*                            geoJson,
  std::string                                tenant,
  const std::vector<std::string>&            servicePathV,
  const std::string&                         apiVersion,
  OrionError*                                oe
)
{
  EntityId*                            eP              = &cerP->contextElement.entityId;
  std::string                          entityId        = cerP->contextElement.entityId.id;
  std::string                          entityType      = cerP->contextElement.entityId.type;
  bool                                 entityModified  = false;
  std::map<std::string, unsigned int>  deletedAttributesCounter;  // Aux var for DELETE operations
  std::vector<std::string>             modifiedAttrs;

  for (unsigned int ix = 0; ix < ceP->contextAttributeVector.size(); ++ix)
  {
    ContextAttribute*  targetAttr = ceP->contextAttributeVector[ix];

    if (targetAttr->skip == true)
    {
      continue;
    }

    /* No matter if success or fail, we have to include the attribute in the response */
    ContextAttribute*  ca         = new ContextAttribute(targetAttr->name, targetAttr->type, "");

    setResponseMetadata(targetAttr, ca);
    cerP->contextElement.contextAttributeVector.push_back(ca);

    /* actualUpdate could be changed to false in the "update" case (or "append as update"). For "delete" and
     * "append" it would keep the true value untouched */
    bool actualUpdate = true;
    if ((strcasecmp(action.c_str(), "update")) == 0 || (strcasecmp(action.c_str(), "replace")) == 0)
    {
      if (!updateContextAttributeItem(cerP,
                                      ca,
                                      attrs,
                                      targetAttr,
                                      notifyCerP,
                                      eP,
                                      toSet,
                                      toPush,
                                      actualUpdate,
                                      entityModified,
                                      currentLocAttrName,
                                      geoJson,
                                      strcasecmp(action.c_str(), "replace") == 0,
                                      apiVersion,
                                      oe))
      {
        return false;
      }
    }
    else if ((strcasecmp(action.c_str(), "append") == 0) || (strcasecmp(action.c_str(), "append_strict") == 0))
    {
      if (!appendContextAttributeItem(cerP,
                                      attrs,
                                      targetAttr,
                                      notifyCerP,
                                      eP,
                                      toSet,
                                      toPush,
                                      actualUpdate,
                                      entityModified,
                                      currentLocAttrName,
                                      geoJson,
                                      apiVersion,
                                      oe))
      {
        return false;
      }
    }
    else if (strcasecmp(action.c_str(), "delete") == 0)
    {
      if (!deleteContextAttributeItem(cerP,
                                      ca,
                                      attrs,
                                      targetAttr,
                                      notifyCerP,
                                      eP,
                                      toUnset,
                                      entityModified,
                                      currentLocAttrName,
                                      &deletedAttributesCounter,
                                      apiVersion,
                                      oe))
      {
        return false;
      }
    }
    else
    {
      std::string details = std::string("unknown actionType: '") + action + "'";
      cerP->statusCode.fill(SccInvalidParameter, details);
      oe->fill(SccBadRequest, details, "BadRequest");

      // This is a BUG in the parse layer checks
      LM_E(("Runtime Error (unknown actionType '%s')", action.c_str()));
      return false;
    }

    /* Add the attribute to the list of modifiedAttrs, in order to check at the end if it triggers some
     * ONCHANGE subscription. Note that actualUpdate is always true in the case of  "delete" or "append",
     * so the if statement is "bypassed" */
    if (actualUpdate)
    {
      modifiedAttrs.push_back(ca->name);
    }
  }

  /* Special processing in the case of DELETE, to avoid "deleting too much" in the case of using
   * metadata ID. If metadata ID feature were removed, this ugly piece of code wouldn't be needed ;)
   * FIXME P3: note that in the case of Active/Active configuration it could happen that
   * the attrsName get desynchronized, e.g. starting situation is A-ID1 and A-ID2, node
   * #1 process DELETE A-ID1 and node #2 process DELETE A-ID2, each one thinking than the
   * other A-IDx copy will be there at the end, thus nobody includes it in the toPull array
   */
  if (strcasecmp(action.c_str(), "delete") == 0)
  {
    std::map<string, unsigned int>::iterator it;

    for (it = deletedAttributesCounter.begin(); it != deletedAttributesCounter.end(); ++it)
    {
      std::string  attrName     = it->first;
      unsigned int deletedTimes = it->second;

      if (howManyAttrs(attrs, attrName) <= deletedTimes)
      {
        toPull->append(attrName);
      }
    }
  }

  /* Add triggered ONCHANGE subscriptions */
  std::string err;

  if (!addTriggeredSubscriptions(entityId, entityType, modifiedAttrs, subsToNotify, err, tenant, servicePathV))
  {
    cerP->statusCode.fill(SccReceiverInternalError, err);
    oe->fill(SccReceiverInternalError, err, "InternalServerError");
    return false;
  }

#if 0
  if (!entityModified)
  {
    /* In this case, there wasn't any failure, but ceP was not set. We need to do it ourselves, as the function caller will
     * do a 'continue' without setting it.
     */

    // FIXME P5: this is ugly, our code should be improved to set cerP in a common place for the "happy case"

    cerP->statusCode.fill(SccOk);
  }
#endif

  /* If the status code was not touched (filled with an error), then set it with Ok */
  if (cerP->statusCode.code == SccNone)
  {
    cerP->statusCode.fill(SccOk);
  }

  return entityModified;
}


/* ****************************************************************************
*
* createEntity -
*
*/
static bool createEntity
(
  EntityId*                        eP,
  const ContextAttributeVector&    attrsV,
  int                              now,
  std::string*                     errDetail,
  std::string                      tenant,
  const std::vector<std::string>&  servicePathV,
  const std::string&               apiVersion,
  OrionError*                      oe
)
{
  LM_T(LmtMongo, ("Entity not found in '%s' collection, creating it", getEntitiesCollectionName(tenant).c_str()));

  /* Actually we don't know if this is the first entity (thus, the collection is being created) or not. However, we can
   * invoke ensureLocationIndex() in anycase, given that it is harmless in the case the collection and index already
   * exits (see docs.mongodb.org/manual/reference/method/db.collection.ensureIndex/)*/
  ensureLocationIndex(tenant);

  if (!legalIdUsage(attrsV))
  {
    *errDetail =
      "Attributes with same name with ID and not ID at the same time "
      "in the same entity are forbidden: entity: [" + eP->toString() + "]";

    oe->fill(SccInvalidModification, *errDetail, "Unprocessable");
    return false;
  }

  /* Search for a potential location attribute */
  std::string     locAttr;
  BSONObjBuilder  geoJson;

  if (!processLocationAtEntityCreation(attrsV, &locAttr, &geoJson, errDetail, apiVersion, oe))
  {
    // oe->fill() already managed by processLocationAtEntityCreation()
    return false;
  }

  BSONObjBuilder    attrsToAdd;
  BSONArrayBuilder  attrNamesToAdd;

  for (unsigned int ix = 0; ix < attrsV.size(); ++ix)
  {
    std::string     attrId = attrsV[ix]->getId();
    BSONObjBuilder  bsonAttr;

    if (!attrsV[ix]->typeGiven && (apiVersion == "v2"))
    {
      bsonAttr.append(ENT_ATTRS_TYPE, DEFAULT_TYPE);
    }
    else
    {
      bsonAttr.append(ENT_ATTRS_TYPE, attrsV[ix]->type);
    }

    bsonAttr.append(ENT_ATTRS_CREATION_DATE, now);
    bsonAttr.append(ENT_ATTRS_MODIFICATION_DATE, now);

    attrsV[ix]->valueBson(bsonAttr);

    std::string effectiveName = dbDotEncode(attrsV[ix]->name);
    if (attrId.length() != 0)
    {
      effectiveName += "__" + attrId;
    }

    LM_T(LmtMongo, ("new attribute: {name: %s, type: %s, value: %s}",
                    effectiveName.c_str(),
                    attrsV[ix]->type.c_str(),
                    attrsV[ix]->getValue().c_str()));

    /* Custom metadata */
    BSONObj mdV;
    if (contextAttributeCustomMetadataToBson(mdV, attrsV[ix], apiVersion == "v2"))
    {
      bsonAttr.appendArray(ENT_ATTRS_MD, mdV);
    }

    attrsToAdd.append(effectiveName, bsonAttr.obj());
    attrNamesToAdd.append(attrsV[ix]->name);
  }

  BSONObjBuilder bsonId;

  bsonId.append(ENT_ENTITY_ID, eP->id);

  if (eP->type == "")
  {
    if (apiVersion == "v2")
    {
      // NGSIv2 uses default entity type
      bsonId.append(ENT_ENTITY_TYPE, DEFAULT_TYPE);
    }
  }
  else
  {
    bsonId.append(ENT_ENTITY_TYPE, eP->type);
  }

  bsonId.append(ENT_SERVICE_PATH, servicePathV[0] == ""? DEFAULT_SERVICE_PATH_UPDATES : servicePathV[0]);

  BSONObjBuilder insertedDoc;

  insertedDoc.append("_id", bsonId.obj());
  insertedDoc.append(ENT_ATTRNAMES, attrNamesToAdd.arr());
  insertedDoc.append(ENT_ATTRS, attrsToAdd.obj());
  insertedDoc.append(ENT_CREATION_DATE, now);
  insertedDoc.append(ENT_MODIFICATION_DATE, now);

  /* Add location information in the case it was found */
  if (locAttr.length() > 0)
  {    
    insertedDoc.append(ENT_LOCATION, BSON(ENT_LOCATION_ATTRNAME << locAttr <<
                                          ENT_LOCATION_COORDS   << geoJson.obj()));
  }

  if (!collectionInsert(getEntitiesCollectionName(tenant), insertedDoc.obj(), errDetail))
  {
    oe->fill(SccReceiverInternalError, *errDetail, "InternalError");
    return false;
  }

  return true;
}


/* ****************************************************************************
*
* removeEntity -
*
*/
static bool removeEntity
(
  const std::string&       entityId,
  const std::string&       entityType,
  ContextElementResponse*  cerP,
  const std::string&       tenant,
  const std::string&       servicePath,
  OrionError*              oe
)
{
  const std::string    idString          = "_id." ENT_ENTITY_ID;
  const std::string    typeString        = "_id." ENT_ENTITY_TYPE;
  const std::string    servicePathString = "_id." ENT_SERVICE_PATH;
  BSONObjBuilder       bob;

  bob.append(idString, entityId);
  if (entityType == "")
  {
    bob.append(typeString, BSON("$exists" << false));
  }
  else
  {
    bob.append(typeString, entityType);
  }

  if (servicePath == "")
  {
    bob.append(servicePathString, BSON("$exists" << false));
  }
  else
  {
    bob.append(servicePathString, servicePath);
  }

  std::string err;
  if (!collectionRemove(getEntitiesCollectionName(tenant), bob.obj(), &err))
  {
    cerP->statusCode.fill(SccReceiverInternalError, err);
    oe->fill(SccReceiverInternalError, err, "InternalServerError");
    return false;
  }

  cerP->statusCode.fill(SccOk);
  return true;
}


/* ****************************************************************************
*
* searchContextProviders -
*
*/
void searchContextProviders
(
  const std::string&              tenant,
  const std::vector<std::string>& servicePathV,
  EntityId&                       en,
  ContextAttributeVector&         caV,
  ContextElementResponse*         cerP
)
{
  ContextRegistrationResponseVector  crrV;
  EntityIdVector                     enV;
  AttributeList                      attrL;
  std::string                        err;

  /* Fill input data for registrationsQuery() */
  enV.push_back(&en);
  for (unsigned int ix = 0; ix < caV.size(); ++ix)
  {
    attrL.push_back(caV[ix]->name);
  }

  /* First CPr lookup (in the case some CER is not found): looking in E-A registrations */
  if (someContextElementNotFound(*cerP))
  {
    if (registrationsQuery(enV, attrL, &crrV, &err, tenant, servicePathV, 0, 0, false))
    {
      if (crrV.size() > 0)
      {
        fillContextProviders(cerP, crrV);
      }
    }
    else
    {
      //
      // Unlike errors in DB at entitiesQuery(), DB failure at registrationsQuery()
      // is not considered "critical"
      //
      alarmMgr.dbError(err);
    }
    crrV.release();
  }

  /* Second CPr lookup (in the case some element stills not being found): looking in E-<null> registrations */
  AttributeList attrNullList;
  if (someContextElementNotFound(*cerP))
  {
    if (registrationsQuery(enV, attrNullList, &crrV, &err, tenant, servicePathV, 0, 0, false))
    {
      if (crrV.size() > 0)
      {
        fillContextProviders(cerP, crrV);
      }
    }
    else
    {
      //
      // Unlike errors in DB at entitiesQuery(), DB failure at registrationsQuery()
      // is not considered "critical"
      //
      alarmMgr.dbError(err);
    }
    crrV.release();
  }
}



/* ****************************************************************************
*
* forwardsPending - 
*/
static bool forwardsPending(UpdateContextResponse* upcrsP)
{
  for (unsigned int cerIx = 0; cerIx < upcrsP->contextElementResponseVector.size(); ++cerIx)
  {
    ContextElementResponse* cerP  = upcrsP->contextElementResponseVector[cerIx];

    for (unsigned int aIx = 0 ; aIx < cerP->contextElement.contextAttributeVector.size(); ++aIx)
    {
      ContextAttribute* aP  = cerP->contextElement.contextAttributeVector[aIx];
      
      if (aP->providingApplication.get() != "")
      {
        return true;
      }      
    }
  }

  return false;
}

/* ****************************************************************************
*
* updateEntity -
*/
static void updateEntity
(
  const BSONObj&                  r,
  const std::string&              action,
  const std::string&              tenant,
  const std::vector<std::string>& servicePathV,
  const std::string&              xauthToken,
  ContextElement*                 ceP,
  UpdateContextResponse*          responseP,
  bool*                           attributeAlreadyExistsError,
  std::string*                    attributeAlreadyExistsList,
  const std::string&              apiVersion,
  const std::string&              fiwareCorrelator
)
{
  // Used to accumulate error response information
  *attributeAlreadyExistsError         = false;
  *attributeAlreadyExistsList          = "[ ";

  EntityId*          enP               = &ceP->entityId;
  const std::string  idString          = "_id." ENT_ENTITY_ID;
  const std::string  typeString        = "_id." ENT_ENTITY_TYPE;
  const std::string  servicePathString = "_id." ENT_SERVICE_PATH;

  BSONObj            idField           = getFieldF(r, "_id").embeddedObject();

  std::string        entityId          = getStringFieldF(idField, ENT_ENTITY_ID);
  std::string        entityType        = idField.hasField(ENT_ENTITY_TYPE) ? getStringFieldF(idField, ENT_ENTITY_TYPE) : "";
  std::string        entitySPath       = getStringFieldF(idField, ENT_SERVICE_PATH);

  LM_T(LmtServicePath, ("Found entity '%s' in ServicePath '%s'", entityId.c_str(), entitySPath.c_str()));

  ContextElementResponse* cerP = new ContextElementResponse();
  cerP->contextElement.entityId.fill(entityId, entityType, "false");

  /* If the vector of Context Attributes is empty and the operation was DELETE, then delete the entity */
  if (strcasecmp(action.c_str(), "delete") == 0 && ceP->contextAttributeVector.size() == 0)
  {
    LM_T(LmtServicePath, ("Removing entity"));
    removeEntity(entityId, entityType, cerP, tenant, entitySPath, &(responseP->oe));
    responseP->contextElementResponseVector.push_back(cerP);
    return;
  }

  LM_T(LmtServicePath, ("ceP->contextAttributeVector.size: %d", ceP->contextAttributeVector.size()));
  /* We take as input the attrs array in the entity document and generate two outputs: a
   * BSON object for $set (updates and appends) and a BSON object for $unset (deletes). Note that depending
   * the request one of the BSON objects could be empty (it use to be the $unset one). In addition, for
   * APPEND and DELETE updates we use two arrays to push/pull attributes in the attrsNames vector */
  BSONObj           attrs     = getFieldF(r, ENT_ATTRS).embeddedObject();
  BSONObjBuilder    toSet;
  BSONObjBuilder    toUnset;
  BSONArrayBuilder  toPush;
  BSONArrayBuilder  toPull;

  /* We accumulate the subscriptions in a map. The key of the map is the string representing
   * subscription id */
  std::map<string, TriggeredSubscription*> subsToNotify;

  /* Is the entity using location? In that case, we fill the locAttr and currentGeoJson attributes with that information, otherwise
   * we fill an empty locAttrs. Any case, processContextAttributeVector uses that information (and eventually modifies) while it
   * processes the attributes in the updateContext */
  std::string     locAttr = "";
  BSONObj         currentGeoJson;
  BSONObjBuilder  geoJson;

  if (r.hasField(ENT_LOCATION))
  {
    BSONObj loc    = getObjectFieldF(r, ENT_LOCATION);

    locAttr        = getStringFieldF(loc, ENT_LOCATION_ATTRNAME);
    currentGeoJson = getObjectFieldF(loc, ENT_LOCATION_COORDS);
  }

  //
  // Before calling processContextAttributeVector and actually do the work, let's check if the
  // request is of type 'append-only' and if we have any problem with attributes already existing.
  //
  if (strcasecmp(action.c_str(), "append_strict") == 0)
  {
    for (unsigned int ix = 0; ix < ceP->contextAttributeVector.size(); ++ix)
    {
      if (howManyAttrs(attrs, ceP->contextAttributeVector[ix]->name) != 0)
      {
        alarmMgr.badInput(clientIp, "attribute already exists");
        *attributeAlreadyExistsError = true;

        //
        // This attribute should now be removed from the 'query' ...
        // processContextAttributeVector looks at the 'skip' field
        //
        ceP->contextAttributeVector[ix]->skip = true;

        // Add to the list of existing attributes - for the error response
        if (*attributeAlreadyExistsList != "[ ")
        {
          *attributeAlreadyExistsList += ", ";
        }
        *attributeAlreadyExistsList += ceP->contextAttributeVector[ix]->name;
      }
    }
    *attributeAlreadyExistsList += " ]";
  }

  /* Build CER used for notifying (if needed) */
  AttributeList emptyAttrL;
  ContextElementResponse* notifyCerP = new ContextElementResponse(r, emptyAttrL);

  // The hasField() check is needed as the entity could have been created with very old Orion version not
  // supporting modification/creation dates
  notifyCerP->contextElement.creDate = r.hasField(ENT_CREATION_DATE)     ? getIntOrLongFieldAsLongF(r, ENT_CREATION_DATE)     : -1;
  notifyCerP->contextElement.modDate = r.hasField(ENT_MODIFICATION_DATE) ? getIntOrLongFieldAsLongF(r, ENT_MODIFICATION_DATE) : -1;

  if (!processContextAttributeVector(ceP,
                                     action,
                                     subsToNotify,
                                     notifyCerP,
                                     attrs,
                                     &toSet,
                                     &toUnset,
                                     &toPush,
                                     &toPull,
                                     cerP,
                                     &locAttr,
                                     &geoJson,
                                     tenant,
                                     servicePathV,
                                     apiVersion,
                                     &(responseP->oe)))
  {
    // The entity wasn't actually modified, so we don't need to update it and we can continue with the next one

    //
    // FIXME P8: the same three statements are at the end of the while loop. Refactor the code to have this
    // in only one place
    //
    searchContextProviders(tenant, servicePathV, *enP, ceP->contextAttributeVector, cerP);

    if (!(attributeAlreadyExistsError && (strcasecmp(action.c_str(), "append_strict") == 0)))
    {
      // Note that CER generation in the case of attributeAlreadyExistsError has its own logic at
      // processContextElement() function so we need to skip this addition or we will get duplicated
      // CER
      responseP->contextElementResponseVector.push_back(cerP);
    }
    else
    {
      delete cerP;
    }

    releaseTriggeredSubscriptions(subsToNotify);

    notifyCerP->release();
    delete notifyCerP;

    return;
  }

  /* Compose the final update on database */
  LM_T(LmtServicePath, ("Updating the attributes of the ContextElement"));

  if (strcasecmp(action.c_str(), "replace") != 0)
  {
    int now = getCurrentTime();
    toSet.append(ENT_MODIFICATION_DATE, now);
    notifyCerP->contextElement.modDate = now;
  }

  // FIXME P5 https://github.com/telefonicaid/fiware-orion/issues/1142:
  // not sure how the following behaves in the case of "replace"...
  if (locAttr.length() > 0)
  {
    BSONObj newGeoJson = geoJson.obj();

    // If processContextAttributeVector() didn't touched the geoJson, then we
    // use the existing object
    BSONObj finalGeoJson = newGeoJson.nFields() > 0 ? newGeoJson : currentGeoJson;

    toSet.append(ENT_LOCATION, BSON(ENT_LOCATION_ATTRNAME << locAttr <<
                                    ENT_LOCATION_COORDS   << finalGeoJson));
  }
  else
  {
    toUnset.append(ENT_LOCATION, 1);
  }

  /* FIXME: I don't like the obj() step, but it seems to be the only possible way, let's wait for the answer to
   * http://stackoverflow.com/questions/29668439/get-number-of-fields-in-bsonobjbuilder-object */
  BSONObjBuilder  updatedEntity;
  BSONObj         toSetObj    = toSet.obj();
  BSONObj         toUnsetObj  = toUnset.obj();
  BSONArray       toPushArr   = toPush.arr();
  BSONArray       toPullArr   = toPull.arr();

  if (strcasecmp(action.c_str(), "replace") == 0)
  {
    // toSet: { A1: { ... }, A2: { ... } }
    int now = getCurrentTime();
    updatedEntity.append("$set", BSON(ENT_ATTRS << toSetObj << ENT_ATTRNAMES << toPushArr << ENT_MODIFICATION_DATE << now));

    notifyCerP->contextElement.modDate = now;
  }
  else
  {
    // toSet:  { attrs.A1: { ... }, attrs.A2: { ... } }
    if (toSetObj.nFields() > 0)
    {
      updatedEntity.append("$set", toSetObj);
    }

    if (toUnsetObj.nFields() > 0)
    {
      updatedEntity.append("$unset", toUnsetObj);
    }

    if (toPushArr.nFields() > 0)
    {
      updatedEntity.append("$addToSet", BSON(ENT_ATTRNAMES << BSON("$each" << toPushArr)));
    }

    if (toPullArr.nFields() > 0)
    {
      updatedEntity.append("$pullAll", BSON(ENT_ATTRNAMES << toPullArr));
    }
  }

  BSONObj updatedEntityObj = updatedEntity.obj();

  /* Note that the query that we build for updating is slighty different than the query used
   * for selecting the entities to process. In particular, the "no type" branch in the if
   * sentence selects precisely the entity with no type, using the {$exists: false} clause */
  BSONObjBuilder query;

  // idString, typeString from earlier in this function
  query.append(idString, entityId);

  if (entityType == "")
  {
    query.append(typeString, BSON("$exists" << false));
  }
  else
  {
    query.append(typeString, entityType);
  }

  // Servicepath
  query.append(servicePathString, fillQueryServicePath(servicePathV));

  std::string err;
  if (!collectionUpdate(getEntitiesCollectionName(tenant), query.obj(), updatedEntityObj, false, &err))
  {
    cerP->statusCode.fill(SccReceiverInternalError, err);
    responseP->oe.fill(SccReceiverInternalError, err, "InternalServerError");

    responseP->contextElementResponseVector.push_back(cerP);

    releaseTriggeredSubscriptions(subsToNotify);

    notifyCerP->release();
    delete notifyCerP;

    return;
  }

  /* Send notifications for each one of the ONCHANGE subscriptions accumulated by
   * previous addTriggeredSubscriptions() invocations */
  processSubscriptions(subsToNotify, notifyCerP, &err, tenant, xauthToken, fiwareCorrelator);
  notifyCerP->release();
  delete notifyCerP;

  //
  // processSubscriptions cleans up the triggered subscriptions; this call here to
  // 'releaseTriggeredSubscriptions' is just an extra life-line.
  // Especially it makes us have all the cleanup of the triggered subscriptions in
  // ONE function.
  // The memory to free is allocated in the function addTriggeredSubscriptions.
  //
  releaseTriggeredSubscriptions(subsToNotify);


  /* To finish with this entity processing, search for CPrs in not found attributes and
   * add the corresponding ContextElementResponse to the global response */
  searchContextProviders(tenant, servicePathV, *enP, ceP->contextAttributeVector, cerP);

  // StatusCode may be set already (if so, we keep the existing value)
  if (cerP->statusCode.code == SccNone)
  {
    cerP->statusCode.fill(SccOk);
  }
  responseP->contextElementResponseVector.push_back(cerP);
}

/* ****************************************************************************
*
* contextElementPreconditionsCheck -
*/
static bool contextElementPreconditionsCheck
(
  ContextElement*         ceP,
  UpdateContextResponse*  responseP,
  const std::string&      action,
  const std::string&      apiVersion
)
{
  /* Getting the entity in the request (helpful in other places) */
  EntityId* enP = &ceP->entityId;

  /* Checking there aren't duplicate attributes */
  for (unsigned int ix = 0; ix < ceP->contextAttributeVector.size(); ++ix)
  {
    std::string name = ceP->contextAttributeVector[ix]->name;
    std::string id   = ceP->contextAttributeVector[ix]->getId();
    for (unsigned int jx = ix + 1; jx < ceP->contextAttributeVector.size(); ++jx)
    {
      if ((name == ceP->contextAttributeVector[jx]->name) && (id == ceP->contextAttributeVector[jx]->getId()))
      {
        ContextAttribute* ca = new ContextAttribute(ceP->contextAttributeVector[ix]);
        std::string details = std::string("duplicated attribute name: name=<") + name + "> id=<" + id + ">";
        alarmMgr.badInput(clientIp, details);
        buildGeneralErrorResponse(ceP, ca, responseP, SccInvalidModification,
                                  "duplicated attribute /" + name + "/");
        responseP->oe.fill(SccBadRequest, "duplicated attribute /" + name + "/", "BadRequest");
        return false; // Error already in responseP
      }
    }
  }

  /* Not supporting isPattern = true currently */
  if (isTrue(enP->isPattern))
  {
    buildGeneralErrorResponse(ceP, NULL, responseP, SccNotImplemented);
    // No need of filling responseP->oe, this cannot happen in NGSIv2
    return false;  // Error already in responseP
  }

  /* Check that UPDATE or APPEND is not used with empty attributes (i.e. no value, no type, no metadata) */
  /* Only wanted for API version v1                                                                      */
  if (((strcasecmp(action.c_str(), "update") == 0) ||
      (strcasecmp(action.c_str(), "append") == 0) ||
      (strcasecmp(action.c_str(), "append_strict") == 0) ||
       (strcasecmp(action.c_str(), "replace") == 0)) && (apiVersion == "v1"))
  {

    // FIXME: Careful, in V2, this check is not wanted ...

    for (unsigned int ix = 0; ix < ceP->contextAttributeVector.size(); ++ix)
    {
      ContextAttribute* aP = ceP->contextAttributeVector[ix];
      if (attributeValueAbsent(aP, apiVersion) && attributeTypeAbsent(aP) && (aP->metadataVector.size() == 0))
      {
        ContextAttribute* ca = new ContextAttribute(aP);

        std::string details = std::string("action: ") + action +
            " - entity: [" + enP->toString(true) + "]" +
            " - offending attribute: " + aP->name +
            " - empty attribute not allowed in APPEND or UPDATE";

        buildGeneralErrorResponse(ceP, ca, responseP, SccInvalidModification, details);
        responseP->oe.fill(SccBadRequest, details, "BadRequest");

        alarmMgr.badInput(clientIp, "empty attribute not allowed in APPEND or UPDATE");
        return false; // Error already in responseP
      }
    }

  }

  return true;

}

/* ****************************************************************************
*
* processContextElement -
*
* 1. Preconditions
* 2. Get the complete list of entities from mongo
*
*/
void processContextElement
(
  ContextElement*                      ceP,
  UpdateContextResponse*               responseP,
  const std::string&                   action,
  const std::string&                   tenant,
  const std::vector<std::string>&      servicePathV,
  std::map<std::string, std::string>&  uriParams,   // FIXME P7: we need this to implement "restriction-based" filters
  const std::string&                   xauthToken,
  const std::string&                   fiwareCorrelator,
  const std::string&                   apiVersion,
  Ngsiv2Flavour                        ngsiv2Flavour
)
{
  /* Check preconditions */
  if (!contextElementPreconditionsCheck(ceP, responseP, action, apiVersion))
  {
    return; // Error already in responseP
  } 

  /* Find entities (could be several, in the case of no type or isPattern=true) */
  const std::string  idString          = "_id." ENT_ENTITY_ID;
  const std::string  typeString        = "_id." ENT_ENTITY_TYPE;
  const std::string  servicePathString = "_id." ENT_SERVICE_PATH;
  EntityId*          enP               = &ceP->entityId;
  BSONObjBuilder     bob;

  bob.append(idString, enP->id);

  if (enP->type != "")
  {
    bob.append(typeString, enP->type);
  }

  // Service path
  bob.append(servicePathString, fillQueryServicePath(servicePathV));

  // FIXME P7: we build the filter for '?!exist=entity::type' directly at mongoBackend layer given that
  // Restriction is not a valid field in updateContext according to the NGSI specification. In the
  // future we may consider to modify the spec to add such Restriction and avoid this ugly "direct injection"
  // of URI filter into mongoBackend
  //
  if (uriParams[URI_PARAM_NOT_EXIST] == SCOPE_VALUE_ENTITY_TYPE)
  {
    std::string  entityTypeString = std::string("_id.") + ENT_ENTITY_TYPE;
    BSONObj      b                = BSON(entityTypeString << BSON("$exists" << false));

    bob.appendElements(b);
  }

  BSONObj query = bob.obj();

  auto_ptr<DBClientCursor> cursor;

  // Several checkings related with NGSIv2
  if (apiVersion == "v2")
  {
    unsigned long long entitiesNumber;
    std::string        err;

    if (!collectionCount(getEntitiesCollectionName(tenant), query, &entitiesNumber, &err))
    {
      buildGeneralErrorResponse(ceP, NULL, responseP, SccReceiverInternalError, err);
      responseP->oe.fill(SccReceiverInternalError, err, "InternalServerError");
      return;
    }

    // This is the case of POST /v2/entities, in order to check that entity doesn't previously exist
    if ((entitiesNumber > 0) && (ngsiv2Flavour == NGSIV2_FLAVOUR_ONCREATE))
    {
      buildGeneralErrorResponse(ceP, NULL, responseP, SccInvalidModification, "Already Exists");
      responseP->oe.fill(SccInvalidModification, "Already Exists", "Unprocessable");
      return;
    }

    // This is the case of POST /v2/entities/<id>, in order to check that entity previously exist
    if ((entitiesNumber == 0) && (ngsiv2Flavour == NGSIV2_FLAVOUR_ONAPPEND))
    {
      buildGeneralErrorResponse(ceP, NULL, responseP, SccContextElementNotFound, "Entity does not exist");
      responseP->oe.fill(SccContextElementNotFound, "Entity does not exist", "NotFound");
      return;
    }

    // Next block is to avoid that several entities with the same ID get updated at the same time, which is
    // not allowed in NGSIv2. Note that multi-update has been allowed in NGSIv1 (maybe without
    // thinking too much about it, but NGSIv1 behaviour has to be preserved to keep backward compatibility)
    if (entitiesNumber > 1)
    {
      buildGeneralErrorResponse(ceP, NULL, responseP, SccConflict, MORE_MATCHING_ENT);
      responseP->oe.fill(SccConflict, MORE_MATCHING_ENT, "TooManyResults");
      return;
    }

  }

  std::string err;

  TIME_STAT_MONGO_READ_WAIT_START();
  DBClientBase* connection = getMongoConnection();
  if (!collectionQuery(connection, getEntitiesCollectionName(tenant), query, &cursor, &err))
  {
    releaseMongoConnection(connection);
    TIME_STAT_MONGO_READ_WAIT_STOP();
    buildGeneralErrorResponse(ceP, NULL, responseP, SccReceiverInternalError, err);
    responseP->oe.fill(SccReceiverInternalError, err, "InternalServerError");
    return;
  }
  TIME_STAT_MONGO_READ_WAIT_STOP();

  //
  // Going through the list of found entities.
  // As ServicePath cannot be modified, inside this loop nothing will be done
  // about ServicePath (The ServicePath was present in the mongo query to obtain the list)
  //
  // FIXME P6: Once we allow for ServicePath to be modified, this loop must be looked at.
  //

  std::vector<BSONObj> results;
  unsigned int         docs = 0;
  while (moreSafe(cursor))
  {
    BSONObj r;
    if (!nextSafeOrErrorF(cursor, &r, &err))
    {
      LM_E(("Runtime Error (exception in nextSafe(): %s - query: %s)", err.c_str(), query.toString().c_str()));
      continue;
    }
    docs++;
    LM_T(LmtMongo, ("retrieved document [%d]: '%s'", docs, r.toString().c_str()));

    BSONElement idField = getFieldF(r, "_id");

    //
    // BSONElement::eoo returns true if 'not found', i.e. the field "_id" doesn't exist in 'sub'
    //
    // Now, if 'getFieldF(r, "_id")' is not found, if we continue, calling embeddedObject() on it, then we get
    // an exception and the broker crashes.
    //
    if (idField.eoo() == true)
    {
      std::string details = std::string("error retrieving _id field in doc: '") + r.toString() + "'";
      alarmMgr.dbError(details);
      continue;
    }
    // we need to use getOwned() here, otherwise we have empirically found that bad things may happen with long BSONObjs
    // (see http://stackoverflow.com/questions/36917731/context-broker-crashing-with-certain-update-queries)
    results.push_back(r.getOwned());
  }
  releaseMongoConnection(connection);

  LM_T(LmtServicePath, ("Docs found: %d", results.size()));

  // Used to accumulate error response information, checked at the end
  bool         attributeAlreadyExistsError = false;
  std::string  attributeAlreadyExistsList  = "[ ";

  /* Note that the following loop is not executed if result size is 0, which leads to the
   * 'if' just below to create a new entity */
  for (unsigned int ix = 0; ix < results.size(); ix++)
  {
    updateEntity(results[ix],
                 action,
                 tenant,
                 servicePathV,
                 xauthToken,
                 ceP,
                 responseP,
                 &attributeAlreadyExistsError,
                 &attributeAlreadyExistsList,
                 apiVersion,
                 fiwareCorrelator);
  }

  /*
   * If the entity doesn't already exist, we create it. Note that alternatively, we could do a count()
   * before the query() to check this. However this would add a second interaction with MongoDB.   
   */
  if (results.size() == 0)
  {
    /* Here we set the ServicePath if set in the request (if APPEND, of course).
     * Actually, the 'slash-escaped' ServicePath (variable: 'path') is sent to the function createEntity
     * which sets the ServicePath for the entity.
     */

    /* Creating the common part of the response that doesn't depend on the case */
    ContextElementResponse* cerP = new ContextElementResponse();

    cerP->contextElement.entityId.fill(enP->id, enP->type, "false");

    /* All the attributes existing in the request are added to the response with 'found' set to false
     * in the of UPDATE/DELETE and true in the case of APPEND
     */
    bool foundValue = (strcasecmp(action.c_str(), "append") == 0) || (strcasecmp(action.c_str(), "append_strict") == 0);

    for (unsigned int ix = 0; ix < ceP->contextAttributeVector.size(); ++ix)
    {
      ContextAttribute*  caP  = ceP->contextAttributeVector[ix];
      ContextAttribute*  ca   = new ContextAttribute(caP->name, caP->type, "", foundValue);

      setResponseMetadata(caP, ca);
      cerP->contextElement.contextAttributeVector.push_back(ca);
    }

    if ((strcasecmp(action.c_str(), "update") == 0) || (strcasecmp(action.c_str(), "replace") == 0))
    {
      /* In the case of UPDATE or REPLACE we look for context providers */
      searchContextProviders(tenant, servicePathV, *enP, ceP->contextAttributeVector, cerP);
      cerP->statusCode.fill(SccOk);
      responseP->contextElementResponseVector.push_back(cerP);

      //
      // If no context providers found, then the UPDATE was simply for a non-found entity and an error should be returned
      //
      if (forwardsPending(responseP) == false)
      {
        cerP->statusCode.fill(SccContextElementNotFound);
        responseP->oe.fill(SccContextElementNotFound, "No context element found", "NotFound");
      }
    }
    else if (strcasecmp(action.c_str(), "delete") == 0)
    {
      cerP->statusCode.fill(SccContextElementNotFound);
      responseP->oe.fill(SccContextElementNotFound, "The requested entity has not been found. Check type and id", "NotFound");

      responseP->contextElementResponseVector.push_back(cerP);
    }
    else   /* APPEND or APPEND_STRICT */
    {
      std::string  errReason;
      std::string  errDetail;
      int          now = getCurrentTime();

      if (!createEntity(enP, ceP->contextAttributeVector, now, &errDetail, tenant, servicePathV, apiVersion, &(responseP->oe)))
      {
        cerP->statusCode.fill(SccInvalidParameter, errDetail);
        // In this case, responseP->oe is not filled, at createEntity() deals interally with that
      }
      else
      {
        cerP->statusCode.fill(SccOk);

        /* Successful creation: send potential notifications */
        std::map<string, TriggeredSubscription*> subsToNotify;

        std::vector<std::string> attrNames;
        for (unsigned int ix = 0; ix < ceP->contextAttributeVector.size(); ++ix)
        {
          attrNames.push_back(ceP->contextAttributeVector[ix]->name);
        }

        if (!addTriggeredSubscriptions(enP->id,
                                       enP->type,
                                       attrNames,
                                       subsToNotify,
                                       err,
                                       tenant,
                                       servicePathV))
        {
          releaseTriggeredSubscriptions(subsToNotify);
          cerP->statusCode.fill(SccReceiverInternalError, err);
          responseP->oe.fill(SccReceiverInternalError, err, "InternalError");

          responseP->contextElementResponseVector.push_back(cerP);
          return;  // Error already in responseP
        }

        //
        // Build CER used for notifying (if needed). Service Path vector shouldn't have more than
        // one item, so it should be safe to get item 0
        //
        ContextElementResponse* notifyCerP = new ContextElementResponse(ceP, apiVersion == "v2");

        notifyCerP->contextElement.creDate = now;
        notifyCerP->contextElement.modDate = now;

        notifyCerP->contextElement.entityId.servicePath = servicePathV.size() > 0? servicePathV[0] : "";
        processSubscriptions(subsToNotify, notifyCerP, &errReason, tenant, xauthToken, fiwareCorrelator);

        notifyCerP->release();
        delete notifyCerP;
        releaseTriggeredSubscriptions(subsToNotify);
      }

      responseP->contextElementResponseVector.push_back(cerP);
    }
  }

  if (attributeAlreadyExistsError == true)
  {
    std::string details = "one or more of the attributes in the request already exist: " + attributeAlreadyExistsList;
    buildGeneralErrorResponse(ceP, NULL, responseP, SccBadRequest, details);
    responseP->oe.fill(SccInvalidModification, details, "Unprocessable");
  }

  // Response in responseP
}
