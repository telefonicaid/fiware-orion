
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
*/
#include <utility>
#include <map>
#include <string>
#include <vector>
#include <set>
#include <algorithm>

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
#include "orionTypes/UpdateActionType.h"
#include "cache/subCache.h"
#include "rest/StringFilter.h"
#include "ngsi/Scope.h"
#include "rest/uriParamNames.h"

#include "mongoBackend/connectionOperations.h"
#include "mongoBackend/safeMongo.h"
#include "mongoBackend/dbConstants.h"
#include "mongoBackend/dbFieldEncoding.h"
#include "mongoBackend/MongoGlobal.h"
#include "mongoBackend/TriggeredSubscription.h"
#include "mongoBackend/location.h"
#include "mongoBackend/dateExpiration.h"
#include "mongoBackend/compoundValueBson.h"
#include "mongoBackend/MongoCommonUpdate.h"



/* ****************************************************************************
*
* USING
*/
using mongo::BSONObjBuilder;
using mongo::BSONArrayBuilder;
using mongo::BSONObj;
using mongo::BSONType;
using mongo::BSONArray;
using mongo::BSONElement;
using mongo::OID;
using mongo::DBClientCursor;
using mongo::DBClientBase;
using orion::CompoundValueNode;



/* ****************************************************************************
*
* isNotCustomMetadata -
*
* Check that the parameter is a not custom metadata, i.e. one metadata without
* an special semantic to be interpreted by the context broker itself
*
* NGSIv2 builtin metadata (dateCreated, dateModified, etc.) are considered custom
*
* FIXME P6: this function probably could be removed. By the moment we leave it
* and we decide what to do in some time (if we add a new custom metadata it could be
* a convenient placeholder). If kept, it should be moved to another place
* "closer" to metadata
*/
static bool isNotCustomMetadata(std::string md)
{
  // After removing ID and deprecating location, all metadata are custom so
  // we always return false
  return false;
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
* equalMetadataValues -
*/
static bool equalMetadataValues(const BSONObj& md1, const BSONObj& md2)
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
      /* FIXME #643 P6: metadata array/object are now supported, but we haven't
         implemented yet the logic to compare compounds between them
      case Object:
        ...
        break;

       case Array:
        ...
        break;
      */

    case mongo::NumberDouble:
      if (getNumberFieldF(md1, ENT_ATTRS_MD_TYPE) != getNumberFieldF(md2, ENT_ATTRS_MD_TYPE))
      {
        return false;
      }
      break;

    case mongo::Bool:
      if (getBoolFieldF(md1, ENT_ATTRS_MD_TYPE) != getBoolFieldF(md2, ENT_ATTRS_MD_TYPE))
      {
        return false;
      }
      break;

    case mongo::String:
      if (getStringFieldF(md1, ENT_ATTRS_MD_TYPE) != getStringFieldF(md2, ENT_ATTRS_MD_TYPE))
      {
        return false;
      }
      break;

    case mongo::jstNULL:
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
    case mongo::Object:
      ...
      break;

    case mongo::Array:
      ...
      break;
    */

    case mongo::NumberDouble:
      return getNumberFieldF(md1, ENT_ATTRS_MD_VALUE) == getNumberFieldF(md2, ENT_ATTRS_MD_VALUE);

    case mongo::Bool:
      return getBoolFieldF(md1, ENT_ATTRS_MD_VALUE) == getBoolFieldF(md2, ENT_ATTRS_MD_VALUE);

    case mongo::String:
      return getStringFieldF(md1, ENT_ATTRS_MD_VALUE) == getStringFieldF(md2, ENT_ATTRS_MD_VALUE);

    case mongo::jstNULL:
      return getFieldF(md2, ENT_ATTRS_MD_VALUE).isNull();

    default:
      LM_E(("Runtime Error (unknown metadata value type in DB: %d)", getFieldF(md1, ENT_ATTRS_MD_VALUE).type()));
      return false;
  }
}



/* ****************************************************************************
*
* equalMetadata -
*
* Given two metadata object assuming they have the same size (i.e. number of elements). check that all the values
* are equal, returning false otherwise.
*/
static bool equalMetadata(const BSONObj& md1, const BSONObj& md2)
{
  std::set<std::string>  md1Set;

  // Assuming that md1 and md2 are of equal size, we can equally use md1 or md2 for the set
  md1.getFieldNames(md1Set);

  for (std::set<std::string>::iterator i = md1Set.begin(); i != md1Set.end(); ++i)
  {
    std::string currentMd = *i;

    if (!md2.hasField(currentMd))
    {
      return false;
    }

    BSONObj md1Item = getObjectFieldF(md1, currentMd);
    BSONObj md2Item = getObjectFieldF(md2, currentMd);

    if (!equalMetadataValues(md1Item, md2Item))
    {
      return false;
    }
  }

  return true;
}



/* ****************************************************************************
*
* changedAttr -
*/
static bool attrValueChanges(const BSONObj& attr, ContextAttribute* caP, const bool& forcedUpdate, ApiVersion apiVersion)
{
  /* Not finding the attribute field at MongoDB is considered as an implicit "" */
  if (!attr.hasField(ENT_ATTRS_VALUE))
  {
    return (caP->valueType != orion::ValueTypeString || caP->stringValue != "");
  }

  /* No value in the request means that the value stays as it was before, so it is not a change */
  if (caP->valueType == orion::ValueTypeNotGiven)
  {
    return false;
  }

  switch (getFieldF(attr, ENT_ATTRS_VALUE).type())
  {
  case mongo::Object:
  case mongo::Array:
    /* As the compoundValueP has been checked is NULL before invoking this function, finding
     * a compound value in DB means that there is a change */
    return true;

  case mongo::NumberDouble:
    return caP->valueType != orion::ValueTypeNumber || caP->numberValue != getNumberFieldF(attr, ENT_ATTRS_VALUE) || forcedUpdate;

  case mongo::Bool:
    return caP->valueType != orion::ValueTypeBoolean || caP->boolValue != getBoolFieldF(attr, ENT_ATTRS_VALUE) || forcedUpdate;

  case mongo::String:
    return caP->valueType != orion::ValueTypeString || caP->stringValue != getStringFieldF(attr, ENT_ATTRS_VALUE) || forcedUpdate;

  case mongo::jstNULL:
    return caP->valueType != orion::ValueTypeNull;

  default:
    LM_E(("Runtime Error (unknown attribute value type in DB: %d)", getFieldF(attr, ENT_ATTRS_VALUE).type()));
    return false;
  }
}



/* ****************************************************************************
*
* appendMetadata -
*/
static void appendMetadata
(
  BSONObjBuilder*    mdBuilder,
  BSONArrayBuilder*  mdNamesBuilder,
  const Metadata*    mdP,
  bool               useDefaultType
)
{
  std::string type = mdP->type;

  if (!mdP->typeGiven && useDefaultType)
  {
    if ((mdP->compoundValueP == NULL) || (mdP->compoundValueP->valueType != orion::ValueTypeVector))
    {
      type = defaultType(mdP->valueType);
    }
    else
    {
      type = defaultType(orion::ValueTypeVector);
    }
  }

  mdNamesBuilder->append(mdP->name);
  std::string effectiveName = dbDotEncode(mdP->name);

  if (type != "")
  {
    switch (mdP->valueType)
    {
    case orion::ValueTypeString:
      mdBuilder->append(effectiveName, BSON(ENT_ATTRS_MD_TYPE << type << ENT_ATTRS_MD_VALUE << mdP->stringValue));
      return;

    case orion::ValueTypeNumber:
      mdBuilder->append(effectiveName, BSON(ENT_ATTRS_MD_TYPE << type << ENT_ATTRS_MD_VALUE << mdP->numberValue));
      return;

    case orion::ValueTypeBoolean:
      mdBuilder->append(effectiveName, BSON(ENT_ATTRS_MD_TYPE << type << ENT_ATTRS_MD_VALUE << mdP->boolValue));
      return;

    case orion::ValueTypeNull:
      mdBuilder->append(effectiveName, BSON(ENT_ATTRS_MD_TYPE << type << ENT_ATTRS_MD_VALUE << mongo::BSONNULL));
      return;

    case orion::ValueTypeObject:
      if (mdP->compoundValueP->valueType == orion::ValueTypeVector)
      {
        BSONArrayBuilder ba;
        compoundValueBson(mdP->compoundValueP->childV, ba);
        mdBuilder->append(effectiveName, BSON(ENT_ATTRS_MD_TYPE << type << ENT_ATTRS_MD_VALUE << ba.arr()));
      }
      else
      {
        BSONObjBuilder bo;

        compoundValueBson(mdP->compoundValueP->childV, bo);
        mdBuilder->append(effectiveName, BSON(ENT_ATTRS_MD_TYPE << type << ENT_ATTRS_MD_VALUE << bo.obj()));
      }
      break;

    default:
      LM_E(("Runtime Error (unknown metadata type: %d)", mdP->valueType));
    }
  }
  else
  {
    switch (mdP->valueType)
    {
    case orion::ValueTypeString:
      mdBuilder->append(effectiveName, BSON(ENT_ATTRS_MD_VALUE << mdP->stringValue));
      return;

    case orion::ValueTypeNumber:
      mdBuilder->append(effectiveName, BSON(ENT_ATTRS_MD_VALUE << mdP->numberValue));
      return;

    case orion::ValueTypeBoolean:
      mdBuilder->append(effectiveName, BSON(ENT_ATTRS_MD_VALUE << mdP->boolValue));
      return;

    case orion::ValueTypeNull:
      mdBuilder->append(effectiveName, BSON(ENT_ATTRS_MD_VALUE << mongo::BSONNULL));
      return;

    case orion::ValueTypeObject:
      if (mdP->compoundValueP->isVector())
      {
        BSONArrayBuilder ba;

        compoundValueBson(mdP->compoundValueP->childV, ba);
        mdBuilder->append(effectiveName, BSON(ENT_ATTRS_MD_VALUE << ba.arr()));
      }
      else
      {
        BSONObjBuilder bo;

        compoundValueBson(mdP->compoundValueP->childV, bo);
        mdBuilder->append(effectiveName, BSON(ENT_ATTRS_MD_VALUE << bo.obj()));
      }
      break;

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
static bool mergeAttrInfo(const BSONObj& attr, ContextAttribute* caP, BSONObj* mergedAttr, const bool& forcedUpdate, ApiVersion apiVersion)
{
  BSONObjBuilder ab;

  /* 1. Add value, if present in the request (it could be omitted in the case of updating only metadata).
   *    When the value of the attribute is empty (no update needed/wanted), then the value of the attribute is
   *    'copied' from DB to the variable 'ab' and sent back to mongo, to not destroy the value  */
  if (caP->valueType != orion::ValueTypeNotGiven)
  {
    caP->valueBson(ab, getStringFieldF(attr, ENT_ATTRS_TYPE), ngsiv1Autocast && (apiVersion == V1));
  }
  else
  {
    //
    // Slightly different treatment, depending on attribute value type
    // in DB (string, number, boolean, vector or object)
    //
    switch (getFieldF(attr, ENT_ATTRS_VALUE).type())
    {
    case mongo::Object:
      ab.append(ENT_ATTRS_VALUE, getObjectFieldF(attr, ENT_ATTRS_VALUE));
      break;

    case mongo::Array:
      ab.appendArray(ENT_ATTRS_VALUE, getArrayFieldF(attr, ENT_ATTRS_VALUE));
      break;

    case mongo::NumberDouble:
      ab.append(ENT_ATTRS_VALUE, getNumberFieldF(attr, ENT_ATTRS_VALUE));
      break;

    case mongo::Bool:
      ab.append(ENT_ATTRS_VALUE, getBoolFieldF(attr, ENT_ATTRS_VALUE));
      break;

    case mongo::String:
      ab.append(ENT_ATTRS_VALUE, getStringFieldF(attr, ENT_ATTRS_VALUE));
      break;

    case mongo::jstNULL:
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
  BSONObjBuilder   mdBuilder;
  BSONArrayBuilder mdNamesBuilder;

  /* First add the metadata elements coming in the request */
  for (unsigned int ix = 0; ix < caP->metadataVector.size() ; ++ix)
  {
    Metadata* mdP = caP->metadataVector[ix];

    /* Skip not custom metadata */
    if (isNotCustomMetadata(mdP->name))
    {
      continue;
    }

    appendMetadata(&mdBuilder, &mdNamesBuilder, mdP, apiVersion == V2);
  }


  /* Second, for each metadata previously in the metadata vector but *not included in the request*, add it as is */

  int      mdSize = 0;
  BSONObj  md;

  if (attr.hasField(ENT_ATTRS_MD))
  {
    std::set<std::string>  mdsSet;

    md = getFieldF(attr, ENT_ATTRS_MD).embeddedObject();
    md.getFieldNames(mdsSet);

    for (std::set<std::string>::iterator i = mdsSet.begin(); i != mdsSet.end(); ++i)
    {
      std::string  currentMd = *i;
      BSONObj      mdItem    = getObjectFieldF(md, currentMd);
      Metadata     md(currentMd, mdItem);

      mdSize++;

      if (apiVersion != V2 || caP->onlyValue)
      {
        if (!hasMetadata(dbDotDecode(md.name), md.type, caP))
        {
          appendMetadata(&mdBuilder, &mdNamesBuilder, &md, false);
        }
      }

      // Any compound value in md is released by Metadata::~Metadata
    }
  }


  BSONObj mdNew = mdBuilder.obj();

  if (mdNew.nFields() > 0)
  {
    ab.append(ENT_ATTRS_MD, mdNew);
  }
  ab.append(ENT_ATTRS_MDNAMES, mdNamesBuilder.arr());

  /* 4. Add creation date */
  if (attr.hasField(ENT_ATTRS_CREATION_DATE))
  {
    ab.append(ENT_ATTRS_CREATION_DATE, getIntFieldF(attr, ENT_ATTRS_CREATION_DATE));
  }

  /* Was it an actual update? */
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
    actualUpdate = (attrValueChanges(attr, caP, forcedUpdate, apiVersion) ||
                    ((caP->type != "") &&
                     (!attr.hasField(ENT_ATTRS_TYPE) || getStringFieldF(attr, ENT_ATTRS_TYPE) != caP->type) ) ||
                    mdNew.nFields() != mdSize || !equalMetadata(md, mdNew));
  }
  else
  {
    // FIXME #643 P6: in the case of compound value, it's more difficult to know if an attribute
    // has really changed its value (many levels have to be traversed). Until we can develop the
    // matching logic, we consider actualUpdate always true.
    //
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
*/
static bool contextAttributeCustomMetadataToBson
(
  BSONObj*                 md,
  BSONArray*               mdNames,
  const ContextAttribute*  ca,
  bool                     useDefaultType
)
{
  BSONObjBuilder    mdToAdd;
  BSONArrayBuilder  mdNamesToAdd;

  for (unsigned int ix = 0; ix < ca->metadataVector.size(); ++ix)
  {
    const Metadata* md = ca->metadataVector[ix];

    if (!isNotCustomMetadata(md->name))
    {
      appendMetadata(&mdToAdd, &mdNamesToAdd, md, useDefaultType);
      LM_T(LmtMongo, ("new custom metadata: {name: %s, type: %s, value: %s}",
                      md->name.c_str(), md->type.c_str(), md->toStringValue().c_str()));
    }
  }

  *md      = mdToAdd.obj();
  *mdNames = mdNamesToAdd.arr();

  if (md->nFields() > 0)
  {
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
*/
static bool updateAttribute
(
  BSONObj&            attrs,
  BSONObjBuilder*     toSet,
  BSONArrayBuilder*   toPush,
  ContextAttribute*   caP,
  bool*               actualUpdate,
  bool                isReplace,
  const bool&         forcedUpdate,
  ApiVersion          apiVersion
)
{
  *actualUpdate = false;

  std::string effectiveName = dbDotEncode(caP->name);

  if (isReplace)
  {
    BSONObjBuilder newAttr;
    int            now = getCurrentTime();

    *actualUpdate = true;

    std::string attrType;
    if (!caP->typeGiven && (apiVersion == V2))
    {
      if ((caP->compoundValueP == NULL) || (caP->compoundValueP->valueType != orion::ValueTypeVector))
      {
        attrType = defaultType(caP->valueType);
      }
      else
      {
        attrType = defaultType(orion::ValueTypeVector);
      }
    }
    else
    {
      attrType = caP->type;
    }

    newAttr.append(ENT_ATTRS_TYPE, attrType);
    newAttr.append(ENT_ATTRS_CREATION_DATE, now);
    newAttr.append(ENT_ATTRS_MODIFICATION_DATE, now);

    caP->valueBson(newAttr, attrType, ngsiv1Autocast && (apiVersion == V1));

    /* Custom metadata */
    BSONObj    md;
    BSONArray  mdNames;

    if (contextAttributeCustomMetadataToBson(&md, &mdNames, caP, apiVersion == V2))
    {
      newAttr.append(ENT_ATTRS_MD, md);
    }
    newAttr.append(ENT_ATTRS_MDNAMES, mdNames);

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
    BSONObj attr = getObjectFieldF(attrs, effectiveName);

    *actualUpdate = mergeAttrInfo(attr, caP, &newAttr, forcedUpdate, apiVersion);
    if (*actualUpdate)
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
*/
static bool appendAttribute
(
  BSONObj&            attrs,
  BSONObjBuilder*     toSet,
  BSONArrayBuilder*   toPush,
  ContextAttribute*   caP,
  bool*               actualUpdate,
  const bool&         forcedUpdate,
  ApiVersion          apiVersion
)
{
  std::string effectiveName = dbDotEncode(caP->name);

  /* APPEND with existing attribute equals to UPDATE */
  if (attrs.hasField(effectiveName.c_str()))
  {
    updateAttribute(attrs, toSet, toPush, caP, actualUpdate, false, forcedUpdate, apiVersion);
    return false;
  }

  /* Build the attribute to append */
  BSONObjBuilder ab;

  /* 1. Value */
  caP->valueBson(ab, caP->type, ngsiv1Autocast && (apiVersion == V1));

  /* 2. Type */
  if ((apiVersion == V2) && !caP->typeGiven)
  {
    std::string attrType;

    if ((caP->compoundValueP == NULL) || (caP->compoundValueP->valueType != orion::ValueTypeVector))
    {
      attrType = defaultType(caP->valueType);
    }
    else
    {
      attrType = defaultType(orion::ValueTypeVector);
    }

    ab.append(ENT_ATTRS_TYPE, attrType);
  }
  else
  {
    ab.append(ENT_ATTRS_TYPE, caP->type);
  }

  /* 3. Metadata */
  BSONObj   md;
  BSONArray mdNames;

  if (contextAttributeCustomMetadataToBson(&md, &mdNames, caP, apiVersion == V2))
  {
    ab.append(ENT_ATTRS_MD, md);
  }
  ab.append(ENT_ATTRS_MDNAMES, mdNames);

  /* 4. Dates */
  int now = getCurrentTime();
  ab.append(ENT_ATTRS_CREATION_DATE, now);
  ab.append(ENT_ATTRS_MODIFICATION_DATE, now);

  const std::string composedName = std::string(ENT_ATTRS) + "." + effectiveName;
  toSet->append(composedName, ab.obj());
  toPush->append(caP->name);

  *actualUpdate = true;
  return true;
}



/* ****************************************************************************
*
* deleteAttribute -
*
* Returns true if an attribute was deleted, false otherwise
*
*/
static bool deleteAttribute
(
  BSONObj&                              attrs,
  BSONObjBuilder*                       toUnset,
  BSONArrayBuilder*                     toPull,
  ContextAttribute*                     caP
)
{
  std::string effectiveName = dbDotEncode(caP->name);

  if (!attrs.hasField(effectiveName.c_str()))
  {
    return false;
  }

  const std::string composedName = std::string(ENT_ATTRS) + "." + effectiveName;

  toUnset->append(composedName, 1);
  toPull->append(caP->name);

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
static std::string servicePathSubscriptionRegex(const std::string& servicePath, std::vector<std::string>& spathV)
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
*/
static bool addTriggeredSubscriptions_withCache
(
  std::string                                    entityId,
  std::string                                    entityType,
  const std::vector<std::string>&                modifiedAttrs,
  std::map<std::string, TriggeredSubscription*>& subs,
  std::string&                                   err,
  std::string                                    tenant,
  const std::vector<std::string>&                servicePathV
)
{
  std::string                       servicePath = (servicePathV.size() > 0)? servicePathV[0] : "";
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
      LM_T(LmtSubCache, ("%s is EXPIRED (EXP:%lu, NOW:%lu, DIFF: %d)",
                         cSubP->subscriptionId, cSubP->expirationTime, now, now - cSubP->expirationTime));
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
    StringList aList;

    aList.fill(cSubP->attributes);

    // Throttling
    if ((cSubP->throttling != -1) && (cSubP->lastNotificationTime != 0))
    {
      if ((now - cSubP->lastNotificationTime) < cSubP->throttling)
      {
        LM_T(LmtSubCache, ("subscription '%s' ignored due to throttling "
                           "(T: %lu, LNT: %lu, NOW: %lu, NOW-LNT: %lu, T: %lu)",
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
        LM_T(LmtSubCache, ("subscription '%s' NOT ignored due to throttling "
                           "(T: %lu, LNT: %lu, NOW: %lu, NOW-LNT: %lu, T: %lu)",
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
      LM_T(LmtSubCache, ("subscription '%s' NOT ignored due to throttling II "
                         "(T: %lu, LNT: %lu, NOW: %lu, NOW-LNT: %lu, T: %lu)",
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
    subP->metadata  = cSubP->metadata;

    subP->fillExpression(cSubP->expression.georel, cSubP->expression.geometry, cSubP->expression.coords);

    std::string errorString;

    if (!subP->stringFilterSet(&cSubP->expression.stringFilter, &errorString))
    {
      LM_E(("Runtime Error (error setting string filter: %s)", errorString.c_str()));
      delete subP;
      cacheSemGive(__FUNCTION__, "match subs for notifications");
      return false;
    }

    if (!subP->mdStringFilterSet(&cSubP->expression.mdStringFilter, &errorString))
    {
      LM_E(("Runtime Error (error setting metadata string filter: %s)", errorString.c_str()));
      delete subP;
      cacheSemGive(__FUNCTION__, "match subs for notifications");
      return false;
    }

    subs.insert(std::pair<std::string, TriggeredSubscription*>(cSubP->subscriptionId, subP));
  }

  cacheSemGive(__FUNCTION__, "match subs for notifications");
  return true;
}



/* ****************************************************************************
*
* CSubQueryGroup - to avoid allocating too much on the stack in addTriggeredSubscriptions_noCache function
*/
typedef struct CSubQueryGroup
{
  BSONObj         idNPtypeNP;              // First clause: idNPtypeNP

  BSONObj         idPtypeNP;               // Second clause: idPtypeNP
  std::string     functionIdPtypeNP;
  BSONObjBuilder  boPNP;

  BSONObj         idNPtypeP;               // Third clause: idNPtypeP
  std::string     functionIdNPtypeP;
  BSONObjBuilder  boNPP;

  std::string     functionIdPtypeP;        // Fourth clause: idPtypeP
  BSONObj         idPtypeP;
  BSONObjBuilder  boPP;

  BSONObj         query;                   // Final query
} CSubQueryGroup;



/* ****************************************************************************
*
* fill_idNPtypeNP -
*/
static void fill_idNPtypeNP
(
  CSubQueryGroup*     bgP,
  const std::string&  entIdQ,
  const std::string&  entityId,
  const std::string&  entTypeQ,
  const std::string&  entityType,
  const std::string&  entPatternQ,
  const std::string&  typePatternQ,
  const BSONObj&      spBson
)
{
  bgP->idNPtypeNP = BSON(entIdQ << entityId <<
                         "$or" << BSON_ARRAY(BSON(entTypeQ << entityType) <<
                                             BSON(entTypeQ << BSON("$exists" << false))) <<
                         entPatternQ << "false" <<
                         typePatternQ << BSON("$ne" << true) <<
                         CSUB_EXPIRATION   << BSON("$gt" << (long long) getCurrentTime()) <<
                         CSUB_STATUS << BSON("$ne" << STATUS_INACTIVE) <<
                         CSUB_SERVICE_PATH << spBson);
}



/* ****************************************************************************
*
* fill_idPtypeNP -
*/
static void fill_idPtypeNP
(
  CSubQueryGroup*     bgP,
  const std::string&  entityId,
  const std::string&  entityType,
  const std::string&  entPatternQ,
  const std::string&  typePatternQ,
  const BSONObj&      spBson
)
{
  bgP->functionIdPtypeNP = std::string("function()") +
         "{" +
            "for (var i=0; i < this."+CSUB_ENTITIES+".length; i++) {" +
                "if (this."+CSUB_ENTITIES+"[i]."+CSUB_ENTITY_ISPATTERN+" == \"true\" && " +
                    "!this."+CSUB_ENTITIES+"[i]."+CSUB_ENTITY_ISTYPEPATTERN+" && " +
                    "(this."+CSUB_ENTITIES+"[i]."+CSUB_ENTITY_TYPE+" == \""+entityType+"\" || " +
                        "this."+CSUB_ENTITIES+"[i]."+CSUB_ENTITY_TYPE+" == \"\" || " +
                        "!(\""+CSUB_ENTITY_TYPE+"\" in this."+CSUB_ENTITIES+"[i])) && " +
                    "\""+entityId+"\".match(this."+CSUB_ENTITIES+"[i]."+CSUB_ENTITY_ID+")) {" +
                    "return true; " +
                "}" +
            "}" +
            "return false; " +
         "}";
  LM_T(LmtMongo, ("idTtypeNP function: %s", bgP->functionIdPtypeNP.c_str()));

  bgP->boPNP.append(entPatternQ, "true");
  bgP->boPNP.append(typePatternQ, BSON("$ne" << true));
  bgP->boPNP.append(CSUB_EXPIRATION, BSON("$gt" << (long long) getCurrentTime()));
  bgP->boPNP.append(CSUB_STATUS, BSON("$ne" << STATUS_INACTIVE));
  bgP->boPNP.append(CSUB_SERVICE_PATH, spBson);
  bgP->boPNP.appendCode("$where", bgP->functionIdPtypeNP);

  bgP->idPtypeNP = bgP->boPNP.obj();
}



/* ****************************************************************************
*
* fill_idNPtypeP -
*/
static void fill_idNPtypeP
(
  CSubQueryGroup*     bgP,
  const std::string&  entityId,
  const std::string&  entityType,
  const std::string&  entPatternQ,
  const std::string&  typePatternQ,
  const BSONObj&      spBson
)
{
  bgP->functionIdNPtypeP = std::string("function()") +
      "{" +
         "for (var i=0; i < this."+CSUB_ENTITIES+".length; i++) {" +
             "if (this."+CSUB_ENTITIES+"[i]."+CSUB_ENTITY_ISPATTERN+" == \"false\" && " +
                 "this."+CSUB_ENTITIES+"[i]."+CSUB_ENTITY_ISTYPEPATTERN+" && " +
                 "this."+CSUB_ENTITIES+"[i]."+CSUB_ENTITY_ID+" == \""+entityId+"\" && " +
                 "\""+entityType+"\".match(this."+CSUB_ENTITIES+"[i]."+CSUB_ENTITY_TYPE+")) {" +
                 "return true; " +
             "}" +
         "}" +
         "return false; " +
      "}";
  LM_T(LmtMongo, ("idNPtypeP function: %s", bgP->functionIdNPtypeP.c_str()));

  bgP->boNPP.append(entPatternQ, "false");
  bgP->boNPP.append(typePatternQ, true);
  bgP->boNPP.append(CSUB_EXPIRATION, BSON("$gt" << (long long) getCurrentTime()));
  bgP->boNPP.append(CSUB_STATUS, BSON("$ne" << STATUS_INACTIVE));
  bgP->boNPP.append(CSUB_SERVICE_PATH, spBson);
  bgP->boNPP.appendCode("$where", bgP->functionIdNPtypeP);

  bgP->idNPtypeP = bgP->boNPP.obj();
}



/* ****************************************************************************
*
* fill_idPtypeP -
*/
static void fill_idPtypeP
(
  CSubQueryGroup*     bgP,
  const std::string&  entIdQ,
  const std::string&  entityId,
  const std::string&  entTypeQ,
  const std::string&  entityType,
  const std::string&  entPatternQ,
  const std::string&  typePatternQ,
  const BSONObj&      spBson
)
{
  bgP->functionIdPtypeP = std::string("function()") +
      "{" +
         "for (var i=0; i < this."+CSUB_ENTITIES+".length; i++) {" +
             "if (this."+CSUB_ENTITIES+"[i]."+CSUB_ENTITY_ISPATTERN+" == \"true\" && " +
                 "this."+CSUB_ENTITIES+"[i]."+CSUB_ENTITY_ISTYPEPATTERN+" && " +
                 "\""+entityId+"\".match(this."+CSUB_ENTITIES+"[i]."+CSUB_ENTITY_ID+") && " +
                 "\""+entityType+"\".match(this."+CSUB_ENTITIES+"[i]."+CSUB_ENTITY_TYPE+")) {" +
                 "return true; " +
             "}" +
         "}" +
         "return false; " +
      "}";
  LM_T(LmtMongo, ("idPtypeP function: %s", bgP->functionIdPtypeP.c_str()));

  bgP->boPP.append(entPatternQ, "true");
  bgP->boPP.append(typePatternQ, true);
  bgP->boPP.append(CSUB_EXPIRATION, BSON("$gt" << (long long) getCurrentTime()));
  bgP->boPP.append(CSUB_STATUS, BSON("$ne" << STATUS_INACTIVE));
  bgP->boPP.append(CSUB_SERVICE_PATH, spBson);
  bgP->boPP.appendCode("$where", bgP->functionIdPtypeP);

  bgP->idPtypeP = bgP->boPP.obj();
}



/* ****************************************************************************
*
* addTriggeredSubscriptions_noCache
*
*/
static bool addTriggeredSubscriptions_noCache
(
  const std::string&                             entityId,
  const std::string&                             entityType,
  const std::vector<std::string>&                modifiedAttrs,
  std::map<std::string, TriggeredSubscription*>& subs,
  std::string&                                   err,
  const std::string&                             tenant,
  const std::vector<std::string>&                servicePathV
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
  std::string entIdQ        = CSUB_ENTITIES   "." CSUB_ENTITY_ID;
  std::string entTypeQ      = CSUB_ENTITIES   "." CSUB_ENTITY_TYPE;
  std::string entPatternQ   = CSUB_ENTITIES   "." CSUB_ENTITY_ISPATTERN;
  std::string typePatternQ  = CSUB_ENTITIES   "." CSUB_ENTITY_ISTYPEPATTERN;
  std::string inRegex       = "{ $in: [ " + spathRegex + ", null ] }";
  BSONObj     spBson        = mongo::fromjson(inRegex);

  /* Query is an $or of 4 sub-clauses:
   *
   * idNPtypeNP -> id no pattern, type no pattern
   * idPtypeNP  -> id pattern,    type no pattern
   * idNPtypeP  -> id no pattern, type pattern
   * idPtypeP   -> id pattern,    type pattern
   *
   * The last three use $where to search for patterns in DB. As far as I know, this is the only
   * way to do a "reverse regex" query in MongoDB (see http://stackoverflow.com/questions/15966991/mongodb-reverse-regex/15989520).
   * The first part of the "if" in these function is used to ensure that the function matches the corresponding
   * pattern/no-pattern combination, as the entity vector may contain a mix.
   *
   * Note that we are using the construct:
   *
   *   typePatternQ << BSON("$ne" << true)
   *
   * instead of just
   *
   *   typePatternQ << false
   *
   * as the former also matches documents without the typePatternQ (i.e. legacy sub documents created before the
   * isTypePattern feature was developed)
   *
   * FIXME: condTypeQ, condValueQ and servicePath part could be "factorized" out of the $or clause
   */

  //
  // Allocating buffer to hold all these BIG variables, necessary for the population of
  // the four parts of the final query.
  // The necessary variables are too big for the stack and thus moved to the head, inside BsonGroup.
  //
  CSubQueryGroup* bgP = new CSubQueryGroup();

  // Populating bgP with the four clauses
  fill_idNPtypeNP(bgP, entIdQ,   entityId,   entTypeQ,    entityType,   entPatternQ, typePatternQ, spBson);
  fill_idPtypeP(bgP,   entIdQ,   entityId,   entTypeQ,    entityType,   entPatternQ, typePatternQ, spBson);
  fill_idPtypeNP(bgP,  entityId, entityType, entPatternQ, typePatternQ, spBson);
  fill_idNPtypeP(bgP,  entityId, entityType, entPatternQ, typePatternQ, spBson);

  /* Composing final query */
  bgP->query = BSON("$or" << BSON_ARRAY(bgP->idNPtypeNP << bgP->idPtypeNP << bgP->idNPtypeP << bgP->idPtypeP));

  std::string                    collection  = getSubscribeContextCollectionName(tenant);
  std::auto_ptr<DBClientCursor>  cursor;
  std::string                    errorString;

  LM_T(LmtMongo, ("query() in '%s' collection: '%s'",
                  getSubscribeContextCollectionName(tenant).c_str(),
                  bgP->query.toString().c_str()));

  TIME_STAT_MONGO_READ_WAIT_START();
  DBClientBase* connection = getMongoConnection();

  if (collectionQuery(connection, collection, bgP->query, &cursor, &errorString) != true)
  {
    TIME_STAT_MONGO_READ_WAIT_STOP();
    releaseMongoConnection(connection);
    delete bgP;
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
      LM_E(("Runtime Error (exception in nextSafe(): %s - query: %s)", err.c_str(), bgP->query.toString().c_str()));
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
      if (!condValueAttrMatch(sub, modifiedAttrs))
      {
        continue;
      }

      LM_T(LmtMongo, ("adding subscription: '%s'", sub.toString().c_str()));

      //
      // NOTE: renderFormatString: NGSIv1 JSON is 'default' (for old db-content)
      //
      long long         throttling         = sub.hasField(CSUB_THROTTLING)?       getIntOrLongFieldAsLongF(sub, CSUB_THROTTLING)       : -1;
      long long         lastNotification   = sub.hasField(CSUB_LASTNOTIFICATION)? getIntOrLongFieldAsLongF(sub, CSUB_LASTNOTIFICATION) : -1;
      std::string       renderFormatString = sub.hasField(CSUB_FORMAT)? getStringFieldF(sub, CSUB_FORMAT) : "legacy";
      RenderFormat      renderFormat       = stringToRenderFormat(renderFormatString);
      ngsiv2::HttpInfo  httpInfo;

      httpInfo.fill(sub);

      TriggeredSubscription* trigs = new TriggeredSubscription
        (
          throttling,
          lastNotification,
          renderFormat,
          httpInfo,
          subToAttributeList(sub), "", "");

      trigs->blacklist = sub.hasField(CSUB_BLACKLIST)? getBoolFieldF(sub, CSUB_BLACKLIST) : false;

      if (sub.hasField(CSUB_METADATA))
      {
        setStringVectorF(sub, CSUB_METADATA, &(trigs->metadata));
      }

      if (sub.hasField(CSUB_EXPR))
      {
        BSONObj expr = getObjectFieldF(sub, CSUB_EXPR);

        std::string q        = expr.hasField(CSUB_EXPR_Q)      ? getStringFieldF(expr, CSUB_EXPR_Q)      : "";
        std::string mq       = expr.hasField(CSUB_EXPR_MQ)     ? getStringFieldF(expr, CSUB_EXPR_MQ)     : "";
        std::string georel   = expr.hasField(CSUB_EXPR_GEOREL) ? getStringFieldF(expr, CSUB_EXPR_GEOREL) : "";
        std::string geometry = expr.hasField(CSUB_EXPR_GEOM)   ? getStringFieldF(expr, CSUB_EXPR_GEOM)   : "";
        std::string coords   = expr.hasField(CSUB_EXPR_COORDS) ? getStringFieldF(expr, CSUB_EXPR_COORDS) : "";

        trigs->fillExpression(georel, geometry, coords);

        // Parsing q
        if (q != "")
        {
          StringFilter* stringFilterP = new StringFilter(SftQ);

          if (stringFilterP->parse(q.c_str(), &err) == false)
          {
            delete stringFilterP;
            delete bgP;

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
              delete bgP;

              LM_E(("Runtime Error (error setting string filter: %s)", errorString.c_str()));
              releaseMongoConnection(connection);
              return false;
            }

            delete stringFilterP;
          }
        }

        // Parsing mq
        if (mq != "")
        {
          StringFilter* mdStringFilterP = new StringFilter(SftMq);

          if (mdStringFilterP->parse(mq.c_str(), &err) == false)
          {
            delete mdStringFilterP;
            delete bgP;

            LM_E(("Runtime Error (%s)", err.c_str()));
            releaseMongoConnection(connection);
            return false;
          }
          else
          {
            std::string errorString;

            if (!trigs->mdStringFilterSet(mdStringFilterP, &errorString))
            {
              delete mdStringFilterP;
              delete bgP;

              LM_E(("Runtime Error (error setting string filter: %s)", errorString.c_str()));
              releaseMongoConnection(connection);
              return false;
            }

            delete mdStringFilterP;
          }
        }
      }

      subs.insert(std::pair<std::string, TriggeredSubscription*>(subIdStr, trigs));
    }
  }

  releaseMongoConnection(connection);
  delete bgP;

  return true;
}



/* ****************************************************************************
*
* addTriggeredSubscriptions -
*
*/
static bool addTriggeredSubscriptions
(
  const std::string&                             entityId,
  const std::string&                             entityType,
  const std::vector<std::string>&                modifiedAttrs,
  std::map<std::string, TriggeredSubscription*>& subs,
  std::string&                                   err,
  std::string                                    tenant,
  const std::vector<std::string>&                servicePathV
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
  const StringList&                attrL,
  const std::vector<std::string>&  metadataV,
  std::string                      subId,
  RenderFormat                     renderFormat,
  std::string                      tenant,
  const std::string&               xauthToken,
  const std::string&               fiwareCorrelator,
  const ngsiv2::HttpInfo&          httpInfo,
  bool                             blacklist = false
)
{
  NotifyContextRequest   ncr;
  ContextElementResponse cer;

  cer.entity.fill(notifyCerP->entity.id,
                  notifyCerP->entity.type,
                  notifyCerP->entity.isPattern,
                  notifyCerP->entity.servicePath);

  for (unsigned int ix = 0; ix < notifyCerP->entity.attributeVector.size(); ix++)
  {
    ContextAttribute* caP = notifyCerP->entity.attributeVector[ix];

    if ((attrL.size() == 0) || attrL.lookup(ALL_ATTRS) || (blacklist == true))
    {
      /* Empty attribute list in the subscription mean that all attributes are added
       * Note we use cloneCompound=true in the ContextAttribute constructor. This is due to
       * cer.entity destructor does release() on the attrs vector */
      cer.entity.attributeVector.push_back(new ContextAttribute(caP, false, true));
    }
    else
    {
      for (unsigned int jx = 0; jx < attrL.size(); jx++)
      {
        /* 'skip' field is used to mark deleted attributes that must not be included in the
         * notification (see deleteAttrInNotifyCer function for details) */
        if (caP->name == attrL[jx] && !caP->skip)
        {
          /* Note we use cloneCompound=true in the ContextAttribute constructor. This is due to
           * cer.entity destructor does release() on the attrs vector */
          cer.entity.attributeVector.push_back(new ContextAttribute(caP, false, true));
        }
      }
    }
  }

  /* Early exit without sending notification if attribute list is empty */
  if (cer.entity.attributeVector.size() == 0)
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

  ncr.subscriptionId.set(subId);
  getNotifier()->sendNotifyContextRequest(ncr,
                                          httpInfo,
                                          tenant,
                                          xauthToken,
                                          fiwareCorrelator,
                                          renderFormat,
                                          attrL.stringV,
                                          blacklist,
                                          metadataV);
  return true;
}



/* ****************************************************************************
*
* processSubscriptions - send a notification for each subscription in the map
*/
static bool processSubscriptions
(
  std::map<std::string, TriggeredSubscription*>& subs,
  ContextElementResponse*                        notifyCerP,
  std::string*                                   err,
  const std::string&                             tenant,
  const std::string&                             xauthToken,
  const std::string&                             fiwareCorrelator
)
{
  bool ret = true;

  *err = "";

  for (std::map<std::string, TriggeredSubscription*>::iterator it = subs.begin(); it != subs.end(); ++it)
  {
    std::string             mapSubId  = it->first;
    TriggeredSubscription*  tSubP     = it->second;


    /* There are some checks to perform on TriggeredSubscription in order to see if the notification has to be actually sent. Note
     * that checks are done in increasing cost order (e.g. georel check is done at the end).
     *
     * Note that check for triggering based on attributes it isn't part of these checks: it has been already done
     * before adding the subscription to the map.
     */

    /* Check 1: timing (not expired and ok from throttling point of view) */
    if (tSubP->throttling != 1 && tSubP->lastNotification != 1)
    {
      long long  current               = getCurrentTime();
      long long  sinceLastNotification = current - tSubP->lastNotification;

      if (tSubP->throttling > sinceLastNotification)
      {
        LM_T(LmtMongo, ("blocked due to throttling, current time is: %l", current));
        LM_T(LmtSubCache, ("ignored '%s' due to throttling, current time is: %l", tSubP->cacheSubId.c_str(), current));

        continue;
      }
    }

    /* Check 2: String Filters */
    if ((tSubP->stringFilterP != NULL) && (!tSubP->stringFilterP->match(notifyCerP)))
    {
      continue;
    }

    if ((tSubP->mdStringFilterP != NULL) && (!tSubP->mdStringFilterP->match(notifyCerP)))
    {
      continue;
    }

    /* Check 3: expression (georel, which also uses geometry and coords)
     * This should be always the last check, as it is the most expensive one, given that it interacts with DB
     * (Issue #2396 should solve that) */
    if ((tSubP->expression.georel != "") && (tSubP->expression.coords != "") && (tSubP->expression.geometry != ""))
    {
      Scope        geoScope;
      std::string  filterErr;

      if (geoScope.fill(V2, tSubP->expression.geometry, tSubP->expression.coords, tSubP->expression.georel, &filterErr) != 0)
      {
        // This has been already checked at subscription creation/update parsing time. Thus, the code cannot reach
        // this part.
        //
        // (Probably the whole if clause will disapear when the missing part of #1705 gets implemented,
        // moving geo-stuff strings to a filter object in TriggeredSubscription class

        LM_E(("Runtime Error (code cannot reach this point, error: %s)", filterErr.c_str()));
        continue;
      }

      BSONObj areaFilter;
      if (!processAreaScopeV2(&geoScope, &areaFilter))
      {
        // Error in processAreaScopeV2 is interpreted as no-match (conservative approach)
        continue;
      }

      // Look in the database of an entity that maches the geo-filters. Note that this query doesn't
      // check any other filtering condition, assuming they are already checked in other steps.
      std::string  keyId   = "_id." ENT_ENTITY_ID;
      std::string  keyType = "_id." ENT_ENTITY_TYPE;
      std::string  keySp   = "_id." ENT_SERVICE_PATH;
      std::string  keyLoc  = ENT_LOCATION "." ENT_LOCATION_COORDS;
      std::string  id      = notifyCerP->entity.id;
      std::string  type    = notifyCerP->entity.type;
      std::string  sp      = notifyCerP->entity.servicePath;
      BSONObj      query   = BSON(keyId << id << keyType << type << keySp << sp << keyLoc << areaFilter);

      unsigned long long n;
      if (!collectionCount(getEntitiesCollectionName(tenant), query, &n, &filterErr))
      {
        // Error in database access is interpreted as no-match (conservative approach)
        continue;
      }

      // No result? Then no-match
      if (n == 0)
      {
        continue;
      }
    }

    /* Send notification */
    LM_T(LmtSubCache, ("NOT ignored: %s", tSubP->cacheSubId.c_str()));

    bool  notificationSent;

    notificationSent = processOnChangeConditionForUpdateContext(notifyCerP,
                                                                tSubP->attrL,
                                                                tSubP->metadata,
                                                                mapSubId,
                                                                tSubP->renderFormat,
                                                                tenant,
                                                                xauthToken,
                                                                fiwareCorrelator,
                                                                tSubP->httpInfo,
                                                                tSubP->blacklist);

    if (notificationSent)
    {
      long long rightNow = getCurrentTime();
      BSONObj query  = BSON("_id" << OID(mapSubId));
      BSONObj update;
      //
      // If broker running without subscription cache, put lastNotificationTime and count in DB
      //
      if (subCacheActive == false)
      {
        BSONObj subOrig;
        std::string newErr;
        collectionFindOne(getSubscribeContextCollectionName(tenant), query, &subOrig, &newErr);
        std::string status;
        if (!subOrig.isEmpty())
        {
          if (subOrig.hasField(CSUB_STATUS))
          {
            status = getStringFieldF(subOrig, CSUB_STATUS);
          }
        }

        // Update the value of status (in case of oneshot) in DB when broker is running without subscription cache
        if (status == STATUS_ONESHOT)
        {
          update = BSON("$set" << BSON(CSUB_LASTNOTIFICATION << rightNow << CSUB_STATUS << STATUS_INACTIVE) <<
                        "$inc" << BSON(CSUB_COUNT << (long long) 1));
        }
        else
        {
          update = BSON("$set" << BSON(CSUB_LASTNOTIFICATION << rightNow) <<
                        "$inc" << BSON(CSUB_COUNT << (long long) 1));
        }
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
          if (cSubP->status == STATUS_ONESHOT)
          {
            update = BSON("$set" << BSON(CSUB_STATUS << STATUS_INACTIVE));
            // update the status to inactive as status is oneshot (in both DB and csubs cache)
            ret = collectionUpdate(getSubscribeContextCollectionName(tenant), query, update, false, err);
            cSubP->status = STATUS_INACTIVE;

            LM_T(LmtSubCache, ("set status to '%s' as Subscription status is oneshot", cSubP->status.c_str()));
          }
          cSubP->lastNotificationTime = rightNow;
          cSubP->count               += 1;

          LM_T(LmtSubCache, ("set lastNotificationTime to %lu and count to %lu for '%s'",
                             cSubP->lastNotificationTime, cSubP->count, cSubP->subscriptionId));
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

  releaseTriggeredSubscriptions(&subs);

  return ret;
}



/* ****************************************************************************
*
* buildGeneralErrorResponse -
*/
static void buildGeneralErrorResponse
(
  Entity*                 ceP,
  ContextAttribute*       caP,
  UpdateContextResponse*  responseP,
  HttpStatusCode          code,
  std::string             details = "",
  ContextAttributeVector* cavP    = NULL
)
{
  ContextElementResponse* cerP = new ContextElementResponse();

  cerP->entity.fill(ceP->id, ceP->type, ceP->isPattern);

  if (caP != NULL)
  {
    cerP->entity.attributeVector.push_back(caP);
  }
  else if (cavP != NULL)
  {
    cerP->entity.attributeVector.fill(*cavP);
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
  /* Custom metadata (just "mirroring" in the response) */
  for (unsigned int ix = 0; ix < caReq->metadataVector.size(); ++ix)
  {
    Metadata* mdReq = caReq->metadataVector[ix];

    if (!isNotCustomMetadata(mdReq->name))
    {
      Metadata* md = new Metadata(mdReq);
      caRes->metadataVector.push_back(md);
    }
  }
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
  bool                    useDefaultType,
  const std::string&      actionType
)
{
  /* Try to find the attribute in the notification CER */
  for (unsigned int ix = 0; ix < notifyCerP->entity.attributeVector.size(); ix++)
  {
    ContextAttribute* caP = notifyCerP->entity.attributeVector[ix];

    if (caP->name == targetAttr->name)
    {
      if (targetAttr->valueType != orion::ValueTypeNotGiven)
      {
        /* Store previous value (it may be necessary to render previousValue metadata) */
        if (caP->previousValue == NULL)
        {
          caP->previousValue = new ContextAttribute();
        }

        caP->previousValue->type        = caP->type;
        caP->previousValue->valueType   = caP->valueType;
        caP->previousValue->stringValue = caP->stringValue;
        caP->previousValue->boolValue   = caP->boolValue;
        caP->previousValue->numberValue = caP->numberValue;

        if (caP->compoundValueP != NULL)
        {
          // We cannot steal this time, as we are going to steal in a later place (see below)
          caP->previousValue->compoundValueP = caP->compoundValueP->clone();
        }
        else
        {
          caP->previousValue->compoundValueP = NULL;
        }

        /* Set values from target attribute */
        caP->valueType      = targetAttr->valueType;
        caP->stringValue    = targetAttr->stringValue;
        caP->boolValue      = targetAttr->boolValue;
        caP->numberValue    = targetAttr->numberValue;

        // Free memory used by the all compound value (if any)
        if (caP->compoundValueP != NULL)
        {
          delete caP->compoundValueP;
          caP->compoundValueP = NULL;
        }

        // Steal compound value from targetAttr
        caP->compoundValueP        = targetAttr->compoundValueP;
        targetAttr->compoundValueP = NULL;
      }

      /* Set attribute type (except if new value is "", which means that the type is not going to change) */
      if (targetAttr->type != "")
      {
        caP->type = targetAttr->type;
      }

      /* Set actionType */
      caP->actionType = actionType;

      /* Set modification date */
      int now = getCurrentTime();
      caP->modDate = now;

      /* Metadata. Note we clean any previous content as updating an attribute means that all
       * its previous values are removed (this may change if someday PATCH /v2/entities/E/attrs/A/metadata
       * is implemented) */
      // FIXME P3: ensure that location and ID metadata are not problematic (current .test suite doesn't raise
      // any problem, but maybe the specific case is not covered)
      caP->metadataVector.release();
      for (unsigned int jx = 0; jx < targetAttr->metadataVector.size(); jx++)
      {
        Metadata* targetMdP = targetAttr->metadataVector[jx];

        /* Search for matching metadata in the CER attribute */
        // FIXME P5: maybe this can be optimized, as we started with an empty caP->metadataVector so probably
        // we can assume that every metadata is new and the "for" block is not needed (although I'm
        // not fully sure... we should test)
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

            // Free old value of compound, if any
            if (mdP->compoundValueP != NULL)
            {
              delete mdP->compoundValueP;
              mdP->compoundValueP = NULL;
            }

            // Steal compound value from targetMdP
            mdP->compoundValueP       = targetMdP->compoundValueP;
            targetMdP->compoundValueP = NULL;

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

  int now = getCurrentTime();
  caP->creDate = now;
  caP->modDate = now;

  if (caP->compoundValueP)
  {
    // The ContextAttribute constructor steals the compound, but in this case, it must be cloned
    targetAttr->compoundValueP = caP->compoundValueP->clone();
  }

  /* Set actionType */
  caP->actionType = actionType;

  notifyCerP->entity.attributeVector.push_back(caP);
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
  for (unsigned int ix = 0; ix < notifyCerP->entity.attributeVector.size(); ix++)
  {
    ContextAttribute* caP = notifyCerP->entity.attributeVector[ix];
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
  const std::string&        entityDetail,
  BSONObjBuilder*           toSet,
  BSONArrayBuilder*         toPush,
  bool*                     actualUpdate,
  bool*                     entityModified,
  std::string*              currentLocAttrName,
  BSONObjBuilder*           geoJson,
  mongo::Date_t*            dateExpiration,
  bool*                     dateExpirationInPayload,
  bool                      isReplace,
  const bool&               forcedUpdate,
  ApiVersion                apiVersion,
  OrionError*               oe
)
{
  std::string err;

  if (updateAttribute(attrs, toSet, toPush, targetAttr, actualUpdate, isReplace, forcedUpdate, apiVersion))
  {
    // Attribute was found
    *entityModified = (*actualUpdate) || (*entityModified);
  }
  else
  {
    // Attribute was not found
    if (!isReplace)
    {
      /* If updateAttribute() returns false, then that particular attribute has not
       * been found. In this case, we interrupt the processing and early return with
       * an error StatusCode */

      //
      // FIXME P10: not sure if this .fill() is useless... it seems it is "overriden" by
      // another .fill() in this function caller. We keep it by the moment, but it probably
      // will removed when we refactor this function
      //
      std::string details = std::string("action: UPDATE") +
                            " - entity: [" + entityDetail + "]" +
                            " - offending attribute: " + targetAttr->getName();

      cerP->statusCode.fill(SccInvalidParameter, details);
      oe->fill(SccContextElementNotFound, ERROR_DESC_NOT_FOUND_ATTRIBUTE, ERROR_NOT_FOUND);

      /* Although 'ca' has been already pushed into cerP, the pointer is still valid, of course */
      ca->found = false;
    }
  }
  /* Check aspects related with location and date expiration */
  if (!processLocationAtUpdateAttribute(currentLocAttrName, targetAttr, geoJson, &err, apiVersion, oe)
    || !processDateExpirationAtUpdateAttribute(targetAttr, dateExpiration, dateExpirationInPayload, &err, oe))
  {
    std::string details = std::string("action: UPDATE") +
                          " - entity: [" + entityDetail + "]" +
                          " - offending attribute: " + targetAttr->getName() +
                          " - " + err;

    cerP->statusCode.fill(SccInvalidParameter, details);
    // oe->fill() not used, as this is done internally in processLocationAtUpdateAttribute()

    alarmMgr.badInput(clientIp, err);
    return false;
  }

  updateAttrInNotifyCer(notifyCerP, targetAttr, apiVersion == V2, NGSI_MD_ACTIONTYPE_UPDATE);

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
  const std::string&        entityDetail,
  BSONObjBuilder*           toSet,
  BSONArrayBuilder*         toPush,
  bool*                     actualUpdate,
  bool*                     entityModified,
  std::string*              currentLocAttrName,
  BSONObjBuilder*           geoJson,
  mongo::Date_t*            dateExpiration,
  const bool&               forcedUpdate,
  ApiVersion                apiVersion,
  OrionError*               oe
)
{
  std::string err;

  bool actualAppend = appendAttribute(attrs, toSet, toPush, targetAttr, actualUpdate, forcedUpdate, apiVersion);

  *entityModified = (*actualUpdate) || (*entityModified);

  /* Check aspects related with location */
  if (!processLocationAtAppendAttribute(currentLocAttrName, targetAttr, actualAppend, geoJson,
                                        &err, apiVersion, oe)
      || !processDateExpirationAtAppendAttribute(dateExpiration, targetAttr, actualAppend, &err, oe))
  {
    std::string details = std::string("action: APPEND") +
                          " - entity: [" + entityDetail + "]" +
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

  std::string actionType = (actualAppend == true)? NGSI_MD_ACTIONTYPE_APPEND : NGSI_MD_ACTIONTYPE_UPDATE;
  updateAttrInNotifyCer(notifyCerP, targetAttr, apiVersion == V2, actionType);

  return true;
}



/* ****************************************************************************
*
* deleteContextAttributeItem -
*/
static bool deleteContextAttributeItem
(
  ContextElementResponse*               cerP,
  ContextAttribute*                     ca,
  BSONObj&                              attrs,
  ContextAttribute*                     targetAttr,
  ContextElementResponse*               notifyCerP,
  const std::string&                    entityDetail,
  BSONObjBuilder*                       toUnset,
  BSONArrayBuilder*                     toPull,
  std::string*                          currentLocAttrName,
  bool*                                 entityModified,
  mongo::Date_t*                        dateExpiration,
  ApiVersion                            apiVersion,
  OrionError*                           oe
)
{
  if (deleteAttribute(attrs, toUnset, toPull, targetAttr))
  {
    deleteAttrInNotifyCer(notifyCerP, targetAttr);
    *entityModified = true;

    /* Check aspects related with location */
    if (targetAttr->getLocation(apiVersion).length() > 0)
    {
      std::string details = std::string("action: DELETE") +
                            " - entity: [" + entityDetail + "]" +
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

    /* Check aspects related to date expiration.
     * If the target attr is date expiration, nullifying dateExpiration ACTUAL value is the way
     * of specifying that date expiration field is no longer used */
    if (targetAttr->name == DATE_EXPIRES)
    {
      *dateExpiration = NO_EXPIRATION_DATE;
    }

    ca->found = true;
  }
  else
  {
    /* If deleteAttribute() returns false, then that particular attribute has not
     * been found. In this case, we interrupt the processing and early return with
     * a error StatusCode */
    std::string details = std::string("action: DELETE") +
                          " - entity: [" + entityDetail + "]" +
                          " - offending attribute: " + targetAttr->getName() +
                          " - attribute not found";

    cerP->statusCode.fill(SccInvalidParameter, details);
    oe->fill(SccContextElementNotFound, ERROR_DESC_NOT_FOUND_ATTRIBUTE, ERROR_NOT_FOUND);

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
* Returns true if entity is actually modified, false otherwise (including fail cases)
*/
static bool processContextAttributeVector
(
  Entity*                                         eP,
  ActionType                                      action,
  std::map<std::string, TriggeredSubscription*>&  subsToNotify,
  ContextElementResponse*                         notifyCerP,
  BSONObj&                                        attrs,
  BSONObjBuilder*                                 toSet,
  BSONObjBuilder*                                 toUnset,
  BSONArrayBuilder*                               toPush,
  BSONArrayBuilder*                               toPull,
  ContextElementResponse*                         cerP,
  std::string*                                    currentLocAttrName,
  BSONObjBuilder*                                 geoJson,
  mongo::Date_t*                                  dateExpiration,
  bool*                                           dateExpirationInPayload,
  std::string                                     tenant,
  const std::vector<std::string>&                 servicePathV,
  const bool&                                     forcedUpdate,
  ApiVersion                                      apiVersion,
  bool                                            loopDetected,
  OrionError*                                     oe
)
{
  std::string               entityId        = cerP->entity.id;
  std::string               entityType      = cerP->entity.type;
  std::string               entityDetail    = cerP->entity.toString();
  bool                      entityModified  = false;
  std::vector<std::string>  modifiedAttrs;

  for (unsigned int ix = 0; ix < eP->attributeVector.size(); ++ix)
  {
    ContextAttribute*  targetAttr = eP->attributeVector[ix];

    if (targetAttr->skip == true)
    {
      continue;
    }

    /* No matter if success or fail, we have to include the attribute in the response */
    ContextAttribute*  ca = new ContextAttribute(targetAttr->name, targetAttr->type, "");

    setResponseMetadata(targetAttr, ca);
    cerP->entity.attributeVector.push_back(ca);

    /* actualUpdate could be changed to false in the "update" case (or "append as update"). For "delete" and
     * "append" it would keep the true value untouched */
    bool actualUpdate = true;
    if ((action == ActionTypeUpdate) || (action == ActionTypeReplace))
    {
      if (!updateContextAttributeItem(cerP,
                                      ca,
                                      attrs,
                                      targetAttr,
                                      notifyCerP,
                                      entityDetail,
                                      toSet,
                                      toPush,
                                      &actualUpdate,
                                      &entityModified,
                                      currentLocAttrName,
                                      geoJson,
                                      dateExpiration,
                                      dateExpirationInPayload,
                                      action == ActionTypeReplace,
                                      forcedUpdate,
                                      apiVersion,
                                      oe))
      {
        return false;
      }
    }
    else if ((action == ActionTypeAppend) || (action == ActionTypeAppendStrict))
    {
      if (!appendContextAttributeItem(cerP,
                                      attrs,
                                      targetAttr,
                                      notifyCerP,
                                      entityDetail,
                                      toSet,
                                      toPush,
                                      &actualUpdate,
                                      &entityModified,
                                      currentLocAttrName,
                                      geoJson,
                                      dateExpiration,
                                      forcedUpdate,
                                      apiVersion,
                                      oe))
      {
        return false;
      }
    }
    else if (action == ActionTypeDelete)
    {
      if (!deleteContextAttributeItem(cerP,
                                      ca,
                                      attrs,
                                      targetAttr,
                                      notifyCerP,
                                      entityDetail,
                                      toUnset,
                                      toPull,
                                      currentLocAttrName,
                                      &entityModified,
                                      dateExpiration,
                                      apiVersion,
                                      oe))
      {
        return false;
      }
    }
    else
    {
      std::string details = std::string("unknown actionType");

      cerP->statusCode.fill(SccInvalidParameter, details);
      oe->fill(SccBadRequest, details, "BadRequest");

      // If we reach this point, there's a BUG in the parse layer checks
      LM_E(("Runtime Error (unknown actionType)"));
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

  /* Add triggered subscriptions */
  std::string err;

  if (loopDetected)
  {
    LM_W(("Notification loop detected for entity id <%s> type <%s>, skipping subscription triggering", entityId.c_str(), entityType.c_str()));
  }
  else if (!addTriggeredSubscriptions(entityId, entityType, modifiedAttrs, subsToNotify, err, tenant, servicePathV))
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
  Entity*                          eP,
  const ContextAttributeVector&    attrsV,
  int                              now,
  std::string*                     errDetail,
  std::string                      tenant,
  const std::vector<std::string>&  servicePathV,
  ApiVersion                       apiVersion,
  const std::string&               fiwareCorrelator,
  OrionError*                      oeP
)
{
  LM_T(LmtMongo, ("Entity not found in '%s' collection, creating it", getEntitiesCollectionName(tenant).c_str()));

  /* Actually we don't know if this is the first entity (thus, the collection is being created) or not. However, we can
   * invoke ensureLocationIndex() in anycase, given that it is harmless in the case the collection and index already
   * exits (see docs.mongodb.org/manual/reference/method/db.collection.ensureIndex/) */
  ensureLocationIndex(tenant);
  ensureDateExpirationIndex(tenant);

  /* Search for a potential location attribute */
  std::string     locAttr;
  BSONObjBuilder  geoJson;

  if (!processLocationAtEntityCreation(attrsV, &locAttr, &geoJson, errDetail, apiVersion, oeP))
  {
    // oe->fill() already managed by processLocationAtEntityCreation()
    return false;
  }

  /* Search for a potential date expiration attribute */
  mongo::Date_t dateExpiration = NO_EXPIRATION_DATE;

  if (!processDateExpirationAtEntityCreation(attrsV, &dateExpiration, errDetail, oeP))
  {
    // oeP->fill() already managed by processLocationAtEntityCreation()
    return false;
  }

  BSONObjBuilder    attrsToAdd;
  BSONArrayBuilder  attrNamesToAdd;

  for (unsigned int ix = 0; ix < attrsV.size(); ++ix)
  {
    BSONObjBuilder  bsonAttr;

    std::string attrType;

    if (!attrsV[ix]->typeGiven && (apiVersion == V2))
    {
      if ((attrsV[ix]->compoundValueP == NULL) || (attrsV[ix]->compoundValueP->valueType != orion::ValueTypeVector))
      {
        attrType = defaultType(attrsV[ix]->valueType);
      }
      else
      {
        attrType = defaultType(orion::ValueTypeVector);
      }
    }
    else
    {
      attrType = attrsV[ix]->type;
    }

    bsonAttr.append(ENT_ATTRS_TYPE, attrType);
    bsonAttr.append(ENT_ATTRS_CREATION_DATE, now);
    bsonAttr.append(ENT_ATTRS_MODIFICATION_DATE, now);

    attrsV[ix]->valueBson(bsonAttr, attrType, ngsiv1Autocast && (apiVersion == V1));

    std::string effectiveName = dbDotEncode(attrsV[ix]->name);

    LM_T(LmtMongo, ("new attribute: {name: %s, type: %s, value: %s}",
                    effectiveName.c_str(),
                    attrsV[ix]->type.c_str(),
                    attrsV[ix]->getValue().c_str()));

    /* Custom metadata */
    BSONObj   md;
    BSONArray mdNames;
    if (contextAttributeCustomMetadataToBson(&md, &mdNames, attrsV[ix], apiVersion == V2))
    {
      bsonAttr.append(ENT_ATTRS_MD, md);
    }
    bsonAttr.append(ENT_ATTRS_MDNAMES, mdNames);

    attrsToAdd.append(effectiveName, bsonAttr.obj());
    attrNamesToAdd.append(attrsV[ix]->name);
  }

  BSONObjBuilder bsonId;

  bsonId.append(ENT_ENTITY_ID, eP->id);

  if (eP->type == "")
  {
    if (apiVersion == V2)
    {
      // NGSIv2 uses default entity type
      bsonId.append(ENT_ENTITY_TYPE, DEFAULT_ENTITY_TYPE);
    }
  }
  else
  {
    bsonId.append(ENT_ENTITY_TYPE, eP->type);
  }

  bsonId.append(ENT_SERVICE_PATH, servicePathV[0] == ""? SERVICE_PATH_ROOT : servicePathV[0]);

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

  /* Add date expiration in the case it was found */
  if (dateExpiration != NO_EXPIRATION_DATE)
  {
    insertedDoc.appendDate(ENT_EXPIRATION, dateExpiration);
  }

  // Correlator (for notification loop detection logic)
  insertedDoc.append(ENT_LAST_CORRELATOR, fiwareCorrelator);

  if (!collectionInsert(getEntitiesCollectionName(tenant), insertedDoc.obj(), errDetail))
  {
    oeP->fill(SccReceiverInternalError, *errDetail, "InternalError");
    return false;
  }

  return true;
}



/* ****************************************************************************
*
* removeEntity -
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
*/
static void searchContextProviders
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
  StringList                         attrL;
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
  StringList attrNullList;
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

    for (unsigned int aIx = 0 ; aIx < cerP->entity.attributeVector.size(); ++aIx)
    {
      ContextAttribute* aP  = cerP->entity.attributeVector[aIx];

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
  ActionType                      action,
  const std::string&              tenant,
  const std::vector<std::string>& servicePathV,
  const std::string&              xauthToken,
  Entity*                         eP,
  UpdateContextResponse*          responseP,
  bool*                           attributeAlreadyExistsError,
  std::string*                    attributeAlreadyExistsList,
  const bool&                     forcedUpdate,
  ApiVersion                      apiVersion,
  const std::string&              fiwareCorrelator,
  const std::string&              ngsiV2AttrsFormat
)
{
  // Used to accumulate error response information
  *attributeAlreadyExistsError         = false;
  *attributeAlreadyExistsList          = "[ ";

  const std::string  idString          = "_id." ENT_ENTITY_ID;
  const std::string  typeString        = "_id." ENT_ENTITY_TYPE;
  const std::string  servicePathString = "_id." ENT_SERVICE_PATH;

  BSONObj            idField           = getObjectFieldF(r, "_id");

  std::string        entityId          = getStringFieldF(idField, ENT_ENTITY_ID);
  std::string        entityType        = idField.hasField(ENT_ENTITY_TYPE) ? getStringFieldF(idField, ENT_ENTITY_TYPE) : "";
  std::string        entitySPath       = getStringFieldF(idField, ENT_SERVICE_PATH);

  EntityId en(entityId, entityType);

  LM_T(LmtServicePath, ("Found entity '%s' in ServicePath '%s'", entityId.c_str(), entitySPath.c_str()));

  ContextElementResponse* cerP = new ContextElementResponse();
  cerP->entity.fill(entityId, entityType, "false");

  /* If the vector of Context Attributes is empty and the operation was DELETE, then delete the entity */
  if ((action == ActionTypeDelete) && (eP->attributeVector.size() == 0))
  {
    LM_T(LmtServicePath, ("Removing entity"));
    removeEntity(entityId, entityType, cerP, tenant, entitySPath, &(responseP->oe));
    responseP->contextElementResponseVector.push_back(cerP);
    return;
  }

  LM_T(LmtServicePath, ("eP->attributeVector.size: %d", eP->attributeVector.size()));
  /* We take as input the attrs array in the entity document and generate two outputs: a
   * BSON object for $set (updates and appends) and a BSON object for $unset (deletes). Note that depending
   * the request one of the BSON objects could be empty (it use to be the $unset one). In addition, for
   * APPEND and DELETE updates we use two arrays to push/pull attributes in the attrsNames vector */

  BSONObj           attrs     = getObjectFieldF(r, ENT_ATTRS);
  BSONObjBuilder    toSet;
  BSONObjBuilder    toUnset;
  BSONArrayBuilder  toPush;
  BSONArrayBuilder  toPull;

  /* We accumulate the subscriptions in a map. The key of the map is the string representing
   * subscription id */
  std::map<std::string, TriggeredSubscription*> subsToNotify;

  /* Is the entity using location? In that case, we fill the locAttr and currentGeoJson attributes with that information, otherwise
   * we fill an empty locAttrs. Any case, processContextAttributeVector uses that information (and eventually modifies) while it
   * processes the attributes in the updateContext */
  std::string     locAttr = "";
  BSONObj         currentGeoJson;
  BSONObj         newGeoJson;
  BSONObj         finalGeoJson;
  BSONObjBuilder  geoJson;

  if (r.hasField(ENT_LOCATION))
  {
    BSONObj loc    = getObjectFieldF(r, ENT_LOCATION);

    locAttr        = getStringFieldF(loc, ENT_LOCATION_ATTRNAME);
    currentGeoJson = getObjectFieldF(loc, ENT_LOCATION_COORDS);
  }

  /* Is the entity using date expiration? In that case, we fill the currentdateExpiration attribute with that information.
   * In any case, if the request contains a new date expiration, this will become the current one
   * The dateExpirationInPayload boolean is used in case of replace operation,
   * in order to know that the date is a new one, coming from the input request
   */
  mongo::Date_t currentDateExpiration = NO_EXPIRATION_DATE;
  bool dateExpirationInPayload          = false;

  if (r.hasField(ENT_EXPIRATION))
  {
    currentDateExpiration = getField(r, ENT_EXPIRATION).date();
  }

  //
  // Before calling processContextAttributeVector and actually do the work, let's check if the
  // request is of type 'append-only' and if we have any problem with attributes already existing.
  //
  if (action == ActionTypeAppendStrict)
  {
    for (unsigned int ix = 0; ix < eP->attributeVector.size(); ++ix)
    {
      if (attrs.hasField(eP->attributeVector[ix]->name))
      {
        alarmMgr.badInput(clientIp, "attribute already exists");
        *attributeAlreadyExistsError = true;

        //
        // This attribute should now be removed from the 'query' ...
        // processContextAttributeVector looks at the 'skip' field
        //
        eP->attributeVector[ix]->skip = true;

        // Add to the list of existing attributes - for the error response
        if (*attributeAlreadyExistsList != "[ ")
        {
          *attributeAlreadyExistsList += ", ";
        }
        *attributeAlreadyExistsList += eP->attributeVector[ix]->name;
      }
    }
    *attributeAlreadyExistsList += " ]";
  }

  /* Build CER used for notifying (if needed) */
  StringList               emptyAttrL;
  ContextElementResponse*  notifyCerP = new ContextElementResponse(r, emptyAttrL, true, apiVersion);

  // The hasField() check is needed as the entity could have been created with very old Orion version not
  // supporting modification/creation dates
  notifyCerP->entity.creDate = r.hasField(ENT_CREATION_DATE)     ? getIntOrLongFieldAsLongF(r, ENT_CREATION_DATE)     : -1;
  notifyCerP->entity.modDate = r.hasField(ENT_MODIFICATION_DATE) ? getIntOrLongFieldAsLongF(r, ENT_MODIFICATION_DATE) : -1;

  // The logic to detect notification loops is to check that the correlator in the request differs from the last one seen for the entity and,
  // in addition, the request was sent due to a custom notification
  bool loopDetected = false;
  if ((ngsiV2AttrsFormat == "custom") && (r.hasField(ENT_LAST_CORRELATOR)))
  {
    loopDetected = (getStringFieldF(r, ENT_LAST_CORRELATOR) == fiwareCorrelator);
  }

  if (!processContextAttributeVector(eP,
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
                                     &currentDateExpiration,
                                     &dateExpirationInPayload,
                                     tenant,
                                     servicePathV,
                                     forcedUpdate,
                                     apiVersion,
                                     loopDetected,
                                     &(responseP->oe)))
  {
    // The entity wasn't actually modified, so we don't need to update it and we can continue with the next one

    //
    // FIXME P8: the same three statements are at the end of the while loop. Refactor the code to have this
    // in only one place
    //
    searchContextProviders(tenant, servicePathV, en, eP->attributeVector, cerP);

    if (!(attributeAlreadyExistsError && (action == ActionTypeAppendStrict)))
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

    releaseTriggeredSubscriptions(&subsToNotify);

    notifyCerP->release();
    delete notifyCerP;

    return;
  }

  /* Compose the final update on database */
  LM_T(LmtServicePath, ("Updating the attributes of the ContextElement"));

  if (action != ActionTypeReplace)
  {
    int now = getCurrentTime();
    toSet.append(ENT_MODIFICATION_DATE, now);
    notifyCerP->entity.modDate = now;
  }

  // We don't touch toSet in the replace case, due to
  // the way in which BSON is composed in that case (see below)
  if (locAttr.length() > 0)
  {
    newGeoJson = geoJson.obj();

    // If processContextAttributeVector() didn't touched the geoJson, then we
    // use the existing object
    finalGeoJson = newGeoJson.nFields() > 0 ? newGeoJson : currentGeoJson;

    if (action != ActionTypeReplace)
    {
      toSet.append(ENT_LOCATION, BSON(ENT_LOCATION_ATTRNAME << locAttr <<
                                    ENT_LOCATION_COORDS   << finalGeoJson));
    }
    else if (newGeoJson.nFields() == 0)
    {
      toUnset.append(ENT_LOCATION, 1);
    }
  }
  else if (newGeoJson.nFields() == 0)
  {
    toUnset.append(ENT_LOCATION, 1);
  }

  // We don't touch toSet in the replace case, due to
  // the way in which BSON is composed in that case (see below)
  if ((currentDateExpiration != NO_EXPIRATION_DATE) && (action != ActionTypeReplace))
  {
    toSet.appendDate(ENT_EXPIRATION, currentDateExpiration);
  }
  else if (!dateExpirationInPayload)
  {
    toUnset.append(ENT_EXPIRATION, 1);
  }

  // Correlator (for notification loop detection logic). We don't touch toSet in the replace case, due to
  // the way in which BSON is composed in that case (see below)
  if (action != ActionTypeReplace)
  {
    toSet.append(ENT_LAST_CORRELATOR, fiwareCorrelator);
  }

  /* FIXME: I don't like the obj() step, but it seems to be the only possible way, let's wait for the answer to
   * http://stackoverflow.com/questions/29668439/get-number-of-fields-in-bsonobjbuilder-object */
  BSONObjBuilder  updatedEntity;
  BSONObj         toSetObj    = toSet.obj();
  BSONObj         toUnsetObj  = toUnset.obj();
  BSONArray       toPushArr   = toPush.arr();
  BSONArray       toPullArr   = toPull.arr();

  if (action == ActionTypeReplace)
  {
    // toSet: { A1: { ... }, A2: { ... } }
    BSONObjBuilder replaceSet;
    int            now = getCurrentTime();

    // In order to enable easy append management of fields (e.g. location, dateExpiration),
    // we use a BSONObjBuilder instead the BSON stream macro.
    replaceSet.append(ENT_ATTRS, toSetObj);
    replaceSet.append(ENT_ATTRNAMES, toPushArr);
    replaceSet.append(ENT_MODIFICATION_DATE, now);
    replaceSet.append(ENT_LAST_CORRELATOR, fiwareCorrelator);

    if (dateExpirationInPayload)
    {
      replaceSet.append(ENT_EXPIRATION, currentDateExpiration);
    }

    if (newGeoJson.nFields() > 0)
    {
      replaceSet.append(ENT_LOCATION, BSON(ENT_LOCATION_ATTRNAME << locAttr <<
                                    ENT_LOCATION_COORDS   << finalGeoJson));
    }

    updatedEntity.append("$set", replaceSet.obj());

    if (!dateExpirationInPayload || newGeoJson.nFields() == 0)
    {
      updatedEntity.append("$unset", toUnsetObj);
    }

    notifyCerP->entity.modDate = now;
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

  // Service Path
  query.append(servicePathString, fillQueryServicePath(servicePathV));

  std::string err;
  if (!collectionUpdate(getEntitiesCollectionName(tenant), query.obj(), updatedEntityObj, false, &err))
  {
    cerP->statusCode.fill(SccReceiverInternalError, err);
    responseP->oe.fill(SccReceiverInternalError, err, "InternalServerError");

    responseP->contextElementResponseVector.push_back(cerP);

    releaseTriggeredSubscriptions(&subsToNotify);

    notifyCerP->release();
    delete notifyCerP;

    return;
  }

  /* Send notifications for each one of the subscriptions accumulated by
   * previous addTriggeredSubscriptions() invocations. Before that, we add
   * builtin attributes and metadata (both NGSIv1 and NGSIv2 as this is
   * for notifications and NGSIv2 builtins can be used in NGSIv1 notifications) */
  addBuiltins(notifyCerP);
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
  releaseTriggeredSubscriptions(&subsToNotify);


  /* To finish with this entity processing, search for CPrs in not found attributes and
   * add the corresponding ContextElementResponse to the global response */
  if ((action == ActionTypeUpdate) || (action == ActionTypeReplace))
  {
    searchContextProviders(tenant, servicePathV, en, eP->attributeVector, cerP);
  }

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
  Entity*                 eP,
  UpdateContextResponse*  responseP,
  ActionType              action,
  ApiVersion              apiVersion
)
{
  /* Checking there aren't duplicate attributes */
  for (unsigned int ix = 0; ix < eP->attributeVector.size(); ++ix)
  {
    std::string name = eP->attributeVector[ix]->name;
    for (unsigned int jx = ix + 1; jx < eP->attributeVector.size(); ++jx)
    {
      if ((name == eP->attributeVector[jx]->name))
      {
        ContextAttribute* ca = new ContextAttribute(eP->attributeVector[ix]);
        std::string details = std::string("duplicated attribute name: name=<") + name + ">";
        alarmMgr.badInput(clientIp, details);
        buildGeneralErrorResponse(eP, ca, responseP, SccInvalidModification,
                                  "duplicated attribute /" + name + "/");
        responseP->oe.fill(SccBadRequest, "duplicated attribute /" + name + "/", "BadRequest");
        return false;  // Error already in responseP
      }
    }
  }

  /* Not supporting isPattern = true currently */
  if (isTrue(eP->isPattern))
  {
    buildGeneralErrorResponse(eP, NULL, responseP, SccNotImplemented);
    // No need of filling responseP->oe, this cannot happen in NGSIv2
    return false;  // Error already in responseP
  }

  /* Check that UPDATE or APPEND is not used with empty attributes (i.e. no value, no type, no metadata) */
  /* Only wanted for API version v1                                                                      */
  if (((action == ActionTypeUpdate) ||
       (action == ActionTypeAppend) ||
       (action == ActionTypeAppendStrict) ||
       (action == ActionTypeReplace)) && (apiVersion == V1))
  {
    // FIXME: Careful, in V2, this check is not wanted ...

    for (unsigned int ix = 0; ix < eP->attributeVector.size(); ++ix)
    {
      ContextAttribute* aP = eP->attributeVector[ix];
      if (aP->valueType == orion::ValueTypeNotGiven && aP->type == "" && (aP->metadataVector.size() == 0))
      {
        ContextAttribute* ca = new ContextAttribute(aP);

        std::string details = std::string("action: ") + actionTypeString(apiVersion, action) +
            " - entity: [" + eP->toString(true) + "]" +
            " - offending attribute: " + aP->name +
            " - empty attribute not allowed in APPEND or UPDATE";

        buildGeneralErrorResponse(eP, ca, responseP, SccInvalidModification, details);
        responseP->oe.fill(SccBadRequest, details, "BadRequest");

        alarmMgr.badInput(clientIp, "empty attribute not allowed in APPEND or UPDATE");
        return false;  // Error already in responseP
      }
    }
  }

  return true;
}



/* ****************************************************************************
*
* setActionType -
*/
static void setActionType(ContextElementResponse* notifyCerP, std::string actionType)
{
  for (unsigned int ix = 0; ix < notifyCerP->entity.attributeVector.size(); ix++)
  {
    ContextAttribute* caP = notifyCerP->entity.attributeVector[ix];
    caP->actionType = actionType;
  }
}



/* ****************************************************************************
*
* processContextElement -
*
* 1. Preconditions
* 2. Get the complete list of entities from mongo
*/
void processContextElement
(
  Entity*                              eP,
  UpdateContextResponse*               responseP,
  ActionType                           action,
  const std::string&                   tenant,
  const std::vector<std::string>&      servicePathV,
  std::map<std::string, std::string>&  uriParams,   // FIXME P7: we need this to implement "restriction-based" filters
  const std::string&                   xauthToken,
  const std::string&                   fiwareCorrelator,
  const std::string&                   ngsiV2AttrsFormat,
  const bool&                          forcedUpdate,
  ApiVersion                           apiVersion,
  Ngsiv2Flavour                        ngsiv2Flavour
)
{
  /* Check preconditions */
  if (!contextElementPreconditionsCheck(eP, responseP, action, apiVersion))
  {
    return;  // Error already in responseP
  }

  /* Find entities (could be several, in the case of no type or isPattern=true) */
  const std::string  idString          = "_id." ENT_ENTITY_ID;
  const std::string  typeString        = "_id." ENT_ENTITY_TYPE;
  const std::string  servicePathString = "_id." ENT_SERVICE_PATH;

  EntityId           en(eP->id, eP->type);
  BSONObjBuilder     bob;

  bob.append(idString, eP->id);

  if (eP->type != "")
  {
    bob.append(typeString, eP->type);
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

  BSONObj                        query = bob.obj();
  std::auto_ptr<DBClientCursor>  cursor;

  // Several checks related to NGSIv2
  if (apiVersion == V2)
  {
    unsigned long long entitiesNumber;
    std::string        err;

    if (!collectionCount(getEntitiesCollectionName(tenant), query, &entitiesNumber, &err))
    {
      buildGeneralErrorResponse(eP, NULL, responseP, SccReceiverInternalError, err);
      responseP->oe.fill(SccReceiverInternalError, err, "InternalServerError");
      return;
    }

    // This is the case of POST /v2/entities, in order to check that entity doesn't previously exist
    if ((entitiesNumber > 0) && (ngsiv2Flavour == NGSIV2_FLAVOUR_ONCREATE))
    {
      buildGeneralErrorResponse(eP, NULL, responseP, SccInvalidModification, "Already Exists");
      responseP->oe.fill(SccInvalidModification, "Already Exists", "Unprocessable");
      return;
    }

    // This is the case of POST /v2/entities/<id>, in order to check that entity previously exist
    if ((entitiesNumber == 0) && (ngsiv2Flavour == NGSIV2_FLAVOUR_ONAPPEND))
    {
      buildGeneralErrorResponse(eP, NULL, responseP, SccContextElementNotFound, ERROR_DESC_NOT_FOUND_ENTITY);
      responseP->oe.fill(SccContextElementNotFound, ERROR_DESC_NOT_FOUND_ENTITY, ERROR_NOT_FOUND);
      return;
    }

    // Next block is to avoid that several entities with the same ID get updated at the same time, which is
    // not allowed in NGSIv2. Note that multi-update has been allowed in NGSIv1 (maybe without
    // thinking too much about it, but NGSIv1 behaviour has to be preserved to keep backward compatibility)
    if (entitiesNumber > 1)
    {
      buildGeneralErrorResponse(eP, NULL, responseP, SccConflict, ERROR_DESC_TOO_MANY_ENTITIES);
      responseP->oe.fill(SccConflict, ERROR_DESC_TOO_MANY_ENTITIES, ERROR_TOO_MANY);
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
    buildGeneralErrorResponse(eP, NULL, responseP, SccReceiverInternalError, err);
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

    //
    // We need to use getOwned() here, otherwise we have empirically found that bad things may happen with long BSONObjs
    // (see http://stackoverflow.com/questions/36917731/context-broker-crashing-with-certain-update-queries)
    //
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
                 eP,
                 responseP,
                 &attributeAlreadyExistsError,
                 &attributeAlreadyExistsList,
                 forcedUpdate,
                 apiVersion,
                 fiwareCorrelator,
                 ngsiV2AttrsFormat);
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

    cerP->entity.fill(eP->id, eP->type, "false");

    /* All the attributes existing in the request are added to the response with 'found' set to false
     * in the of UPDATE/DELETE and true in the case of APPEND
     */
    bool foundValue = ((action == ActionTypeAppend) || (action == ActionTypeAppendStrict));

    for (unsigned int ix = 0; ix < eP->attributeVector.size(); ++ix)
    {
      ContextAttribute*  caP  = eP->attributeVector[ix];
      ContextAttribute*  ca   = new ContextAttribute(caP->name, caP->type, "", foundValue);

      setResponseMetadata(caP, ca);
      cerP->entity.attributeVector.push_back(ca);
    }

    if ((action == ActionTypeUpdate) || (action == ActionTypeReplace))
    {
      /* In the case of UPDATE or REPLACE we look for context providers */
      searchContextProviders(tenant, servicePathV, en, eP->attributeVector, cerP);
      cerP->statusCode.fill(SccOk);
      responseP->contextElementResponseVector.push_back(cerP);

      //
      // If no context providers found, then the UPDATE was simply for a non-found entity and an error should be returned
      //
      if (forwardsPending(responseP) == false)
      {
        cerP->statusCode.fill(SccContextElementNotFound);

        if (apiVersion == V1)
        {
          responseP->oe.fill(SccContextElementNotFound, ERROR_DESC_NOT_FOUND_CONTEXT_ELEMENT, ERROR_NOT_FOUND);
        }
        else
        {
          responseP->oe.fill(SccContextElementNotFound, ERROR_DESC_NOT_FOUND_ENTITY, ERROR_NOT_FOUND);
        }
      }
    }
    else if (action == ActionTypeDelete)
    {
      cerP->statusCode.fill(SccContextElementNotFound);

      responseP->oe.fill(SccContextElementNotFound, ERROR_DESC_NOT_FOUND_ENTITY, ERROR_NOT_FOUND);
      responseP->contextElementResponseVector.push_back(cerP);
    }
    else   /* APPEND or APPEND_STRICT */
    {
      std::string  errReason;
      std::string  errDetail;
      int          now = getCurrentTime();

      if (!createEntity(eP, eP->attributeVector, now, &errDetail, tenant, servicePathV, apiVersion, fiwareCorrelator, &(responseP->oe)))
      {
        cerP->statusCode.fill(SccInvalidParameter, errDetail);
        // In this case, responseP->oe is not filled, as createEntity() deals internally with that
      }
      else
      {
        cerP->statusCode.fill(SccOk);

        /* Successful creation: send potential notifications */
        std::map<std::string, TriggeredSubscription*>  subsToNotify;
        std::vector<std::string>                       attrNames;

        for (unsigned int ix = 0; ix < eP->attributeVector.size(); ++ix)
        {
          attrNames.push_back(eP->attributeVector[ix]->name);
        }

        if (!addTriggeredSubscriptions(eP->id,
                                       eP->type,
                                       attrNames,
                                       subsToNotify,
                                       err,
                                       tenant,
                                       servicePathV))
        {
          releaseTriggeredSubscriptions(&subsToNotify);
          cerP->statusCode.fill(SccReceiverInternalError, err);
          responseP->oe.fill(SccReceiverInternalError, err, "InternalError");

          responseP->contextElementResponseVector.push_back(cerP);
          return;  // Error already in responseP
        }

        //
        // Build CER used for notifying (if needed). Service Path vector shouldn't have more than
        // one item, so it should be safe to get item 0
        //
        ContextElementResponse* notifyCerP = new ContextElementResponse(eP, apiVersion == V2);

        // Set action type
        setActionType(notifyCerP, NGSI_MD_ACTIONTYPE_APPEND);

        // Set creaDate and modDate times
        notifyCerP->entity.creDate = now;
        notifyCerP->entity.modDate = now;

        for (unsigned int ix = 0; ix < notifyCerP->entity.attributeVector.size(); ix++)
        {
          ContextAttribute* caP = notifyCerP->entity.attributeVector[ix];
          caP->creDate = now;
          caP->modDate = now;
        }

        notifyCerP->entity.servicePath = servicePathV.size() > 0? servicePathV[0] : "";
        /* Send notifications for each one of the subscriptions accumulated by
         * previous addTriggeredSubscriptions() invocations. Before that, we add
         * builtin attributes and metadata (both NGSIv1 and NGSIv2 as this is
         * for notifications and NGSIv2 builtins can be used in NGSIv1 notifications) */
        addBuiltins(notifyCerP);
        processSubscriptions(subsToNotify, notifyCerP, &errReason, tenant, xauthToken, fiwareCorrelator);

        notifyCerP->release();
        delete notifyCerP;
        releaseTriggeredSubscriptions(&subsToNotify);
      }

      responseP->contextElementResponseVector.push_back(cerP);
    }
  }

  if (attributeAlreadyExistsError == true)
  {
    std::string details = "one or more of the attributes in the request already exist: " + attributeAlreadyExistsList;
    buildGeneralErrorResponse(eP, NULL, responseP, SccBadRequest, details);
    responseP->oe.fill(SccInvalidModification, details, "Unprocessable");
  }

  // Response in responseP
}
