
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

#include "mongoBackend/dbConstants.h"
#include "mongoBackend/dbFieldEncoding.h"
#include "mongoBackend/MongoGlobal.h"
#include "mongoBackend/TriggeredSubscription.h"
#include "mongoBackend/location.h"
#include "mongoBackend/dateExpiration.h"
#include "mongoBackend/compoundValueBson.h"
#include "mongoBackend/MongoCommonUpdate.h"

#include "mongoDriver/connectionOperations.h"
#include "mongoDriver/mongoConnectionPool.h"
#include "mongoDriver/safeMongo.h"
#include "mongoDriver/BSONObjBuilder.h"
#include "mongoDriver/BSONArrayBuilder.h"
#include "mongoDriver/DBErrors.h"


/* ****************************************************************************
*
* USING
*/
using orion::CompoundValueNode;



/* ****************************************************************************
*
* correlatorRoot -
*
* This functions returns the "correlator root", i.e. correlator without the "cbnotif" decorator
*
* For instance if full correlator is
*
*   f320136c-0192-11eb-a893-000c29df7908; cbnotif=32
*
* then the correlator root is:
*
*   f320136c-0192-11eb-a893-000c29df7908
*
* This is needed as the self-notification loop protection logic is based on the correlator root
* so we store that in the DB.
*
*/
inline std::string correlatorRoot(const std::string& fullCorrelator)
{
  size_t p = fullCorrelator.find(";");
  if (p == std::string::npos)
  {
    return fullCorrelator;
  }
  else
  {
    return fullCorrelator.substr(0, p);
  }
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
static bool equalMetadataItems(const orion::BSONObj& md1, const orion::BSONObj& md2)
{
  bool md1TypeExist = md1.hasField(ENT_ATTRS_MD_TYPE);
  bool md2TypeExist = md2.hasField(ENT_ATTRS_MD_TYPE);

  // If type exists in one metadata but not in the other, then the result is unequality
  if ((md1TypeExist && !md2TypeExist) || (!md1TypeExist && md2TypeExist))
  {
    return false;
  }

  // If type exists in both metadata elements, check if they are the same
  if (md1TypeExist && md2TypeExist)
  {
    if ((getFieldF(md1, ENT_ATTRS_MD_TYPE).type() != orion::String))
    {
      LM_E(("Runtime Error (unallowed JSON type for metadata NGSI type: %d)", getFieldF(md1, ENT_ATTRS_MD_TYPE).type()));
      return false;
    }
    if ((getFieldF(md2, ENT_ATTRS_MD_TYPE).type() != orion::String))
    {
      LM_E(("Runtime Error (unallowed JSON type for metadata NGSI type: %d)", getFieldF(md2, ENT_ATTRS_MD_TYPE).type()));
      return false;
    }

    if (getStringFieldF(md1, ENT_ATTRS_MD_TYPE) != getStringFieldF(md2, ENT_ATTRS_MD_TYPE))
    {
      return false;
    }
  }

  // declared types are equal. Same value ?
  if (getFieldF(md1, ENT_ATTRS_MD_VALUE).type() != getFieldF(md2, ENT_ATTRS_MD_VALUE).type())
  {
    return false;
  }

  switch (getFieldF(md1, ENT_ATTRS_MD_VALUE).type())
  {
    case orion::Object:
      return getObjectFieldF(md1, ENT_ATTRS_MD_VALUE).equal(getObjectFieldF(md2, ENT_ATTRS_MD_VALUE));

    case orion::Array:
      return getArrayFieldF(md1, ENT_ATTRS_MD_VALUE).equal(getArrayFieldF(md2, ENT_ATTRS_MD_VALUE));

    case orion::NumberDouble:
      return getNumberFieldF(md1, ENT_ATTRS_MD_VALUE) == getNumberFieldF(md2, ENT_ATTRS_MD_VALUE);

    case orion::Bool:
      return getBoolFieldF(md1, ENT_ATTRS_MD_VALUE) == getBoolFieldF(md2, ENT_ATTRS_MD_VALUE);

    case orion::String:
      return getStringFieldF(md1, ENT_ATTRS_MD_VALUE) == getStringFieldF(md2, ENT_ATTRS_MD_VALUE);

    case orion::jstNULL:
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
static bool equalMetadata(const orion::BSONObj& md1, const orion::BSONObj& md2)
{
  std::set<std::string>  md1Set;

  // Assuming that md1 and md2 are of equal size, we can equally use md1 or md2 for the set
  md1.getFieldNames(&md1Set);

  for (std::set<std::string>::iterator i = md1Set.begin(); i != md1Set.end(); ++i)
  {
    std::string currentMd = *i;

    if (!md2.hasField(currentMd))
    {
      return false;
    }

    orion::BSONObj md1Item = getObjectFieldF(md1, currentMd);
    orion::BSONObj md2Item = getObjectFieldF(md2, currentMd);

    if (!equalMetadataItems(md1Item, md2Item))
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
static bool attrValueChanges(const orion::BSONObj& attr, ContextAttribute* caP)
{
  /* Not finding the attribute field at MongoDB is considered as an implicit "" */
  if (!attr.hasField(ENT_ATTRS_VALUE))
  {
    return (caP->valueType != orion::ValueTypeString || !caP->stringValue.empty());
  }

  /* No value in the request means that the value stays as it was before, so it is not a change */
  if (caP->valueType == orion::ValueTypeNotGiven)
  {
    return false;
  }

  switch (getFieldF(attr, ENT_ATTRS_VALUE).type())
  {
  case orion::Object:
  case orion::Array:
    /* As the compoundValueP has been checked is NULL before invoking this function, finding
     * a compound value in DB means that there is a change */
    return true;

  case orion::NumberDouble:
    return caP->valueType != orion::ValueTypeNumber || caP->numberValue != getNumberFieldF(attr, ENT_ATTRS_VALUE);

  case orion::Bool:
    return caP->valueType != orion::ValueTypeBoolean || caP->boolValue != getBoolFieldF(attr, ENT_ATTRS_VALUE);

  case orion::String:
    return caP->valueType != orion::ValueTypeString || caP->stringValue != getStringFieldF(attr, ENT_ATTRS_VALUE);

  case orion::jstNULL:
    return caP->valueType != orion::ValueTypeNull;

  default:
    LM_E(("Runtime Error (unknown attribute value type in DB: %d on attribute %s)", getFieldF(attr, ENT_ATTRS_VALUE).type(), caP->name.c_str()));
    return false;
  }
}



/* ****************************************************************************
*
* isSomeCalculatedOperatorUsed -
*
* Returns true if some calculated operator ($inc, etc.) is in use, so the
* appending value has to be skipped in mergeAttrInfo (given that a caculateOperatorXXX()
* will include it)
*/
static bool isSomeCalculatedOperatorUsed(ContextAttribute* caP)
{
  if ((caP->compoundValueP == NULL) || (caP->compoundValueP->childV.size() == 0))
  {
    return false;
  }

  for (unsigned ix = 0; ix < UPDATE_OPERATORS_NUMBER; ix++)
  {
    if (caP->compoundValueP->childV[0]->name == UPDATE_OPERATORS[ix])
    {
      return true;
    }
  }

  // $set, $unset, $addToSet and $pullAll are not included in UPDATE_OPERATORS, so they
  // are checked separartelly
  if ((caP->compoundValueP->childV[0]->name == "$set")      ||
      (caP->compoundValueP->childV[0]->name == "$unset")    ||
      (caP->compoundValueP->childV[0]->name == "$addToSet") ||
      (caP->compoundValueP->childV[0]->name == "$pullAll"))
  {
    return true;
  }

  return false;
}



/* ****************************************************************************
*
* ChangeType -
*/
typedef enum ChangeType
{
  NO_CHANGE           = 0,
  CHANGE_ONLY_VALUE   = 1,
  CHANGE_ONLY_MD      = 2,
  CHANGE_VALUE_AND_MD = 3
} ChangeType;

/* ****************************************************************************
*
* mergeAttrInfo -
*
* Takes as input the information of a given attribute, both in database (attr) and
* request (caP), and merged them in the toSet builder. The function returns
* NO_CHANGE, CHANGE_ONLY_VALUE, CHANGE_ONLY_MD and CHANGE_VALUE_AND_MD depending
* of the change type.
*
* You may wonder why we need toUnset if this function is not related with delete attribute
* logic. However, it's need to "clean" metadata in some cases.
*/
static ChangeType mergeAttrInfo
(
  const orion::BSONObj&   attr,
  ContextAttribute*       caP,
  const std::string&      composedName,
  orion::BSONObjBuilder*  toSet,
  orion::BSONObjBuilder*  toUnset,
  const bool&             forcedUpdate,
  const bool&             overrideMetadata,
  ApiVersion              apiVersion
)
{
  /* 1. Add value, if present in the request (it could be omitted in the case of updating only metadata).
   *    When the value of the attribute is empty (no update needed/wanted), then the value of the attribute is
   *    'copied' from DB to the variable 'ab' and sent back to mongo, to not destroy the value  */
  if (caP->valueType != orion::ValueTypeNotGiven)
  {
    // value is omitted from toSet in the case some operator ($inc, etc.) is used
    if (!isSomeCalculatedOperatorUsed(caP))
    {
      // FIXME P7: boolean return value should be managed?
      caP->valueBson(composedName + "." + ENT_ATTRS_VALUE, toSet, getStringFieldF(attr, ENT_ATTRS_TYPE), ngsiv1Autocast && (apiVersion == V1));
    }
  }
  else
  {
    //
    // Slightly different treatment, depending on attribute value type
    // in DB (string, number, boolean, vector or object)
    //
    switch (getFieldF(attr, ENT_ATTRS_VALUE).type())
    {
    case orion::Object:
      toSet->append(composedName + "." + ENT_ATTRS_VALUE, getObjectFieldF(attr, ENT_ATTRS_VALUE));
      break;

    case orion::Array:
      toSet->append(composedName + "." + ENT_ATTRS_VALUE, getArrayFieldF(attr, ENT_ATTRS_VALUE));
      break;

    case orion::NumberDouble:
      toSet->append(composedName + "." + ENT_ATTRS_VALUE, getNumberFieldF(attr, ENT_ATTRS_VALUE));
      break;

    case orion::Bool:
      toSet->append(composedName + "." + ENT_ATTRS_VALUE, getBoolFieldF(attr, ENT_ATTRS_VALUE));
      break;

    case orion::String:
      toSet->append(composedName + "." + ENT_ATTRS_VALUE, getStringFieldF(attr, ENT_ATTRS_VALUE));
      break;

    case orion::jstNULL:
      toSet->appendNull(composedName + "." + ENT_ATTRS_VALUE);
      break;

    default:
      LM_E(("Runtime Error (unknown attribute value type in DB: %d on attribute %s)", getFieldF(attr, ENT_ATTRS_VALUE).type(), caP->name.c_str()));
    }
  }

  /* 2. Add type, if present in request. If not, just use the one that is already present in the database. */
  if (!caP->type.empty())
  {
    toSet->append(composedName + "." + ENT_ATTRS_TYPE, caP->type);
  }
  else
  {
    if (attr.hasField(ENT_ATTRS_TYPE))
    {
      toSet->append(composedName + "." + ENT_ATTRS_TYPE, getStringFieldF(attr, ENT_ATTRS_TYPE));
    }
  }

  /* 3. Add metadata */
  orion::BSONObjBuilder   mdBuilder;
  orion::BSONArrayBuilder mdNamesBuilder;

  /* First add the metadata elements coming in the request */
  for (unsigned int ix = 0; ix < caP->metadataVector.size() ; ++ix)
  {
    Metadata* mdP = caP->metadataVector[ix];

    mdP->appendToBsoN(&mdBuilder, &mdNamesBuilder, apiVersion == V2);
  }


  /* Second, for each metadata previously in the metadata vector but *not included in the request*, add it as is */

  int      mdSize = 0;
  orion::BSONObj  md;

  if (attr.hasField(ENT_ATTRS_MD))
  {
    // FIXME P5: not sure if this way of lookup the metadata collection is the best one
    // or can be simplified
    md = getFieldF(attr, ENT_ATTRS_MD).embeddedObject();
    std::set<std::string>  mdsSet;

    md.getFieldNames(&mdsSet);

    for (std::set<std::string>::iterator i = mdsSet.begin(); i != mdsSet.end(); ++i)
    {
      std::string  currentMd = *i;
      orion::BSONObj      mdItem    = getObjectFieldF(md, currentMd);
      Metadata     md(currentMd, mdItem);

      mdSize++;

      if (apiVersion != V2 || caP->onlyValue || !overrideMetadata)
      {
        if (!hasMetadata(dbDecode(md.name), md.type, caP))
        {
          md.appendToBsoN(&mdBuilder, &mdNamesBuilder, false);
        }
      }

      // Any compound value in md is released by Metadata::~Metadata
    }
  }


  orion::BSONObj mdNew = mdBuilder.obj();

  if (mdNew.nFields() > 0)
  {
    toSet->append(composedName + "." + ENT_ATTRS_MD, mdNew);
  }
  else
  {
    // No metadata, so we remove the field
    toUnset->append(composedName + "." + ENT_ATTRS_MD, 1);
  }
  toSet->append(composedName + "." + ENT_ATTRS_MDNAMES, mdNamesBuilder.arr());

  /* 4. Add creation date */
  if (attr.hasField(ENT_ATTRS_CREATION_DATE))
  {
    toSet->append(composedName + "." + ENT_ATTRS_CREATION_DATE, getNumberFieldF(attr, ENT_ATTRS_CREATION_DATE));
  }

  /* Was it an actual update? */
  ChangeType changeType = NO_CHANGE;

  /* We consider there is a change in the value if one or more of the following are true:
   *
   * 1) forcedUpdate is enabled
   * 2) the value of the attribute changed (see attrValueChanges or CompoundValueNode::equal() for details)
   * 3) the type of the attribute changed (in this case, !attr.hasField(ENT_ATTRS_TYPE) is needed, as attribute
   *    type is optional according to NGSI and the attribute may not have that field in the BSON)
   *
   * In addition, we consider there is change in the metadata if:
   *
   * 3) the metadata changed (this is done checking if the size of the original and final metadata vectors is
   *    different and, if they are of the same size, checking if the vectors are not equal)
   */
  bool valueChanged;
  bool typeChanged;
  bool mdChanged;
  if (caP->compoundValueP == NULL)
  {
    valueChanged = forcedUpdate || attrValueChanges(attr, caP);
  }
  else
  {
    valueChanged = forcedUpdate || !caP->compoundValueP->equal(getFieldF(attr, ENT_ATTRS_VALUE));
  }
  typeChanged = ((!caP->type.empty()) && (!attr.hasField(ENT_ATTRS_TYPE) || getStringFieldF(attr, ENT_ATTRS_TYPE) != caP->type));
  mdChanged = (mdNew.nFields() != mdSize || !equalMetadata(md, mdNew));

  valueChanged = valueChanged || typeChanged;

  if (valueChanged && !mdChanged)
  {
    changeType = CHANGE_ONLY_VALUE;
  }
  else if (!valueChanged && mdChanged)
  {
    changeType = CHANGE_ONLY_MD;
  }
  else if (valueChanged && mdChanged)
  {
    changeType = CHANGE_VALUE_AND_MD;
  }
  else  // !valueChanged && !mdChanged
  {
    changeType = NO_CHANGE;
  }

  /* 5. Add modification date (actual change only if actual update) */
  if (changeType)
  {
    toSet->append(composedName + "." + ENT_ATTRS_MODIFICATION_DATE, getCurrentTime());
  }
  else
  {
    /* The hasField() check is needed to preserve compatibility with entities that were created
     * in database by a CB instance previous to the support of creation and modification dates */
    if (attr.hasField(ENT_ATTRS_MODIFICATION_DATE))
    {
      toSet->append(composedName + "." + ENT_ATTRS_MODIFICATION_DATE, getNumberFieldF(attr, ENT_ATTRS_MODIFICATION_DATE));
    }
  }

  return changeType;
}



/* ****************************************************************************
*
* updateAttribute -
*
* Returns true if an attribute was found, false otherwise. If true,
* the "actualUpdate" argument (passed by reference) is set to true in the case that the
* original value of the attribute was different than the one used in the update (this is
* important for notifications)
*
* The isReplace boolean specifies how toSet has to be filled, either:
*
*   { attrs.A1: { ... }, attrs.A2: { ... } }  (in the case of isReplace = false)
*
* or
*
*   { A1: { ... }, A2: { ... } }              (in the case of isReplace = true)
*
* The former is to be used with { $set: <toSet> }, the later to be used with { attrs: <toSet> }
*
* In addition, in the case of isReplace, the attribute is added to attrNamesAdd (otherwise, attrNamesAdd
* is not touched).
*
* You may wonder why we need toUnset if this function is not related with delete attribute
* logic. However, it's need to "clean" metadata in some cases.
*/
static bool updateAttribute
(
  orion::BSONObj*           attrsP,
  orion::BSONObjBuilder*    toSet,
  orion::BSONObjBuilder*    toUnset,
  orion::BSONArrayBuilder*  attrNamesAdd,
  ContextAttribute*         caP,
  ChangeType*               changeType,
  bool                      isReplace,
  const bool&               forcedUpdate,
  const bool&               overrideMetadata,
  ApiVersion                apiVersion
)
{
  *changeType = NO_CHANGE;

  std::string effectiveName = dbEncode(caP->name);
  const std::string composedName = std::string(ENT_ATTRS) + "." + effectiveName;

  if (isReplace)
  {
    orion::BSONObjBuilder newAttr;
    double         now = getCurrentTime();

    *changeType = CHANGE_VALUE_AND_MD;

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

    // FIXME P7: boolean return value should be managed?
    caP->valueBson(std::string(ENT_ATTRS_VALUE), &newAttr, attrType, ngsiv1Autocast && (apiVersion == V1));

    /* Custom metadata */
    orion::BSONObjBuilder    md;
    orion::BSONArrayBuilder  mdNames;

    caP->metadataVector.toBson(&md, &mdNames, apiVersion == V2);
    if (mdNames.arrSize() > 0)
    {
      newAttr.append(ENT_ATTRS_MD, md.obj());
    }
    newAttr.append(ENT_ATTRS_MDNAMES, mdNames.arr());

    toSet->append(effectiveName, newAttr.obj());
    attrNamesAdd->append(caP->name);
  }
  else
  {
    if (!attrsP->hasField(effectiveName))
    {
      return false;
    }

    orion::BSONObj newAttr;
    orion::BSONObj attr = getObjectFieldF(*attrsP, effectiveName);

    *changeType = mergeAttrInfo(attr, caP, composedName, toSet, toUnset, forcedUpdate, overrideMetadata, apiVersion);
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
* important for notifications). Otherwise it is false
*
* In addition, return value is as follows:
* - true: there was an actual append change
* - false: there was an append-as-update change
*
* You may wonder why we need toUnset if this function is not related with delete attribute
* logic. However, it's need to "clean" metadata in some cases.
*
*/
static bool appendAttribute
(
  orion::BSONObj*           attrsP,
  orion::BSONObjBuilder*    toSet,
  orion::BSONObjBuilder*    toUnset,
  orion::BSONArrayBuilder*  attrNamesAdd,
  ContextAttribute*         caP,
  ChangeType*               changeType,
  const bool&               forcedUpdate,
  const bool&               overrideMetadata,
  ApiVersion                apiVersion
)
{
  std::string effectiveName = dbEncode(caP->name);
  const std::string composedName = std::string(ENT_ATTRS) + "." + effectiveName;

  /* APPEND with existing attribute equals to UPDATE */
  if (attrsP->hasField(effectiveName))
  {
    updateAttribute(attrsP, toSet, toUnset, attrNamesAdd, caP, changeType, false, forcedUpdate, overrideMetadata, apiVersion);
    return false;
  }


  /* 1. Value */
  // value is omitted from toSet in the case some operator ($inc, etc.) is used
  if (!isSomeCalculatedOperatorUsed(caP))
  {
    // FIXME P7: boolean return value should be managed?
    caP->valueBson(composedName + "." + ENT_ATTRS_VALUE, toSet, caP->type, ngsiv1Autocast && (apiVersion == V1));
  }

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

    toSet->append(composedName + "." + ENT_ATTRS_TYPE, attrType);
  }
  else
  {
    toSet->append(composedName + "." + ENT_ATTRS_TYPE, caP->type);
  }

  /* 3. Metadata */
  orion::BSONObjBuilder   md;
  orion::BSONArrayBuilder mdNames;

  caP->metadataVector.toBson(&md, &mdNames, apiVersion == V2);
  if (mdNames.arrSize() > 0)
  {
    toSet->append(composedName + "." + ENT_ATTRS_MD, md.obj());
  }
  toSet->append(composedName + "." + ENT_ATTRS_MDNAMES, mdNames.arr());

  /* 4. Dates */
  double now = getCurrentTime();
  toSet->append(composedName + "." + ENT_ATTRS_CREATION_DATE, now);
  toSet->append(composedName + "." + ENT_ATTRS_MODIFICATION_DATE, now);

  attrNamesAdd->append(caP->name);

  *changeType = CHANGE_VALUE_AND_MD;
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
  orion::BSONObj&           attrs,
  orion::BSONObjBuilder*    toUnset,
  orion::BSONArrayBuilder*  attrNamesRemove,
  ContextAttribute*         caP
)
{
  std::string effectiveName = dbEncode(caP->name);

  if (!attrs.hasField(effectiveName))
  {
    return false;
  }

  const std::string composedName = std::string(ENT_ATTRS) + "." + effectiveName;

  toUnset->append(composedName, 1);
  attrNamesRemove->append(caP->name);

  return true;
}



/* ****************************************************************************
*
* servicePathSubscription -
*
* 1. If the incoming request is without service path, then only subscriptions without
*    service path is a match ('/#' or '/')
* 2. If the incoming request has a service path, then the REGEX must be created:
*    - Incoming: /a1/a2/a3
*    - [ "/#", "/a1/#", "/a1/a2/#", "/a1/a2/a3/#", "/a1/a2/a3" ]
*
*/
static void servicePathSubscription(const std::string& servicePath, orion::BSONArrayBuilder* bab)
{
  std::vector<std::string>  spathV;
  int                       spathComponents = 0;

  //
  // Split Service Path in 'path components'
  //
  if (!servicePath.empty())
  {
    spathComponents = stringSplit(servicePath, '/', spathV);
  }

  if (spathComponents == 0)
  {
    bab->append("/");
    bab->append("/#");
  }
  else
  {
    //
    // 1. '/#'
    //
    bab->append("/#");


    //
    // 2. The whole list /a | /a/b | /a/b/c  etc
    //
    for (int ix = 0; ix < spathComponents; ++ix)
    {
      std::string sp;

      for (int cIx = 0; cIx <= ix; ++cIx)
      {
        sp += std::string("/") + spathV[cIx];
      }

      sp += "/#";

      bab->append(sp);
    }


    //
    // 3. EXACT service path
    //
    bab->append(servicePath);
  }
}



/* ****************************************************************************
*
* addTriggeredSubscriptions_withCache
*/
static bool addTriggeredSubscriptions_withCache
(
  std::string                                    entityId,
  std::string                                    entityType,
  const std::vector<std::string>&                attributes,
  const std::vector<std::string>&                attrsWithModifiedValue,
  const std::vector<std::string>&                attrsWithModifiedMd,
  std::map<std::string, TriggeredSubscription*>& subs,
  std::string&                                   err,
  std::string                                    tenant,
  const std::vector<std::string>&                servicePathV,
  ngsiv2::SubAltType                             targetAltType
)
{
  std::string                       servicePath = (servicePathV.size() > 0)? servicePathV[0] : "";
  std::vector<CachedSubscription*>  subVec;

  cacheSemTake(__FUNCTION__, "match subs for notifications");
  subCacheMatch(tenant.c_str(), servicePath.c_str(), entityId.c_str(), entityType.c_str(), attributes, attrsWithModifiedValue, attrsWithModifiedMd, targetAltType, &subVec);
  LM_T(LmtSubCache, ("%d subscriptions in cache match the update", subVec.size()));

  double now = getCurrentTime();
  for (unsigned int ix = 0; ix < subVec.size(); ++ix)
  {
    CachedSubscription* cSubP = subVec[ix];

    // Outdated subscriptions are skipped
    if (cSubP->expirationTime < now)
    {
      LM_T(LmtSubCache, ("%s is EXPIRED (EXP:%lu, NOW:%f, DIFF: %f)",
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
    if (cSubP->onlyChanged)
    {
      subToNotifyList(cSubP->attributes, attributes, aList, cSubP->blacklist);
      if (aList.size() == 0)
      {
        continue;
      }
    }
    else
    {
      aList.fill(cSubP->attributes);
    }

    // Note that in this case (different from _noCache case) the fails counter in the sum of the
    // last "consolidated" number in DB (if not invalidated by a recent notification success)
    // plus the not yet consolidated fail conunter in cache
    long long failsCounter = cSubP->failsCounter + (cSubP->failsCounterFromDbValid ? cSubP->failsCounterFromDb : 0);
    TriggeredSubscription* subP = new TriggeredSubscription((long long) cSubP->throttling,
                                                           cSubP->maxFailsLimit,
                                                           failsCounter,
                                                           (long long) cSubP->lastNotificationTime,
                                                           cSubP->renderFormat,
                                                           cSubP->httpInfo,
                                                           cSubP->mqttInfo,
                                                           aList,
                                                           cSubP->subscriptionId,
                                                           cSubP->tenant,
                                                           cSubP->covered);
    if (cSubP->onlyChanged)
    {
      subP->blacklist = false;
    }
    else
    {
      subP->blacklist = cSubP->blacklist;
    }
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
  orion::BSONObj         idNPtypeNP;  // First clause: idNPtypeNP

  orion::BSONObj         idPtypeNP;   // Second clause: idPtypeNP
  orion::BSONObjBuilder  boPNP;

  orion::BSONObj         idNPtypeP;   // Third clause: idNPtypeP
  orion::BSONObjBuilder  boNPP;

  orion::BSONObj         idPtypeP;    // Fourth clause: idPtypeP
  orion::BSONObjBuilder  boPP;

  orion::BSONObj         query;       // Final query
} CSubQueryGroup;



/* ****************************************************************************
*
* fill_idNPtypeNP -
*/
static void fill_idNPtypeNP
(
  CSubQueryGroup*     bgP,
  const std::string&  entityId,
  const std::string&  entityType
)
{
  /* Example of query block generated by this function:
   *
   * {
   *   "entities.id": "(entityId),
   *   "$or": [
   *     {
   *       "entities.type": "(entityType)"
   *     },
   *     {
   *       "entities.type": {
   *         "$exists": false
   *       }
   *     }
   *   ],
   *   "entities.isPattern": "false",
   *   "entities.isTypePattern": {
   *     "$ne": true
   *   },
   *   "expiration": {
   *     "$gt": 1666606572
   *   },
   *   "status": {
   *     "$ne": "inactive"
   *   }
   * }
   *
   * Note that we are using the construct:
   *
   *   "entities.isTypePattern": {
   *     "$ne": true
   *   }
   *
   * instead of just
   *
   *   "entities.isTypePattern": false
   *
   * as the former also matches documents without the entities.isTypePattern (i.e. legacy sub
   * documents created before the isTypePattern feature was developed)
   */

  orion::BSONObjBuilder bob;

  orion::BSONArrayBuilder baOr;
  orion::BSONObjBuilder   bobEntityType;
  orion::BSONObjBuilder   bobEntityTypeExistFalse;
  orion::BSONObjBuilder   bobExistFalse;

  orion::BSONObjBuilder bobNeTrue;
  orion::BSONObjBuilder bobGtCurrentTime;
  orion::BSONObjBuilder bobNeStatus;

  bobExistFalse.append("$exists", false);

  bobEntityType.append(CSUB_ENTITIES "." CSUB_ENTITY_TYPE, entityType);
  bobEntityTypeExistFalse.append(CSUB_ENTITIES "." CSUB_ENTITY_TYPE, bobExistFalse.obj());

  baOr.append(bobEntityType.obj());
  baOr.append(bobEntityTypeExistFalse.obj());

  bobNeTrue.append("$ne", true);
  bobGtCurrentTime.append("$gt", (long long) getCurrentTime());
  bobNeStatus.append("$ne", STATUS_INACTIVE);

  bob.append(CSUB_ENTITIES "." CSUB_ENTITY_ID, entityId);
  bob.append("$or", baOr.arr());
  bob.append(CSUB_ENTITIES "." CSUB_ENTITY_ISPATTERN, "false");
  bob.append(CSUB_ENTITIES "." CSUB_ENTITY_ISTYPEPATTERN, bobNeTrue.obj());
  bob.append(CSUB_EXPIRATION, bobGtCurrentTime.obj());
  bob.append(CSUB_STATUS, bobNeStatus.obj());

  bgP->idNPtypeNP = bob.obj();
}



/* ****************************************************************************
*
* fill_idPtypeNP -
*/
static void fill_idPtypeNP
(
  CSubQueryGroup*     bgP,
  const std::string&  entityId,
  const std::string&  entityType
)
{
  /*
   * Given a subscription document, for each item within the entities array field, the condition is true if:
   *
   * 1. the item isPattern field is "true" AND
   * 2. the item isTypePattern field is false OR doesn't exist AND
   * 3. the item type is empty string OR doesn't exist OR is equal to the target type (entityType) AND
   * 4. the item id matches the target id (entityId)
   *
   * 2, 3 and 4 are within the same $expr clause.
   *
   * Example of query block generated by this function:
   *
   * {
   *   "entities.isPattern": "true",
   *   "expiration": {
   *     "$gt": 1666606572
   *   },
   *   "status": {
   *     "$ne": "inactive"
   *   },
   *   "$expr": {
   *     "$anyElementTrue": {
   *       "$map": {
   *         "input": "$entities",
   *         "in": {
   *           "$and": [
   *             {
   *               "$regexMatch": {
   *                 "input": "(entityId)",
   *                 "regex": "$$this.id",
   *                 "options": "i"
   *               }
   *             },
   *             {
   *               "$or": [
   *                 {
   *                   "$eq": [
   *                     "$$this.type",
   *                     "(entityType)"
   *                   ]
   *                 },
   *                 {
   *                   "$eq": [
   *                     "$$this.type",
   *                     ""
   *                   ]
   *                 },
   *                 {
   *                   "$eq": [
   *                     {
   *                       "$type": "$$this.type"
   *                     },
   *                     "missing"
   *                   ]
   *                 }
   *               ]
   *             },
   *             {
   *               "$or": [
   *                 {
   *                   "$eq": [
   *                     "$$this.isTypePattern",
   *                     false
   *                   ]
   *                 },
   *                 {
   *                   "$eq": [
   *                     {
   *                       "$type": "$$this.isTypePattern"
   *                     },
   *                     "missing"
   *                   ]
   *                 }
   *               ]
   *             }
   *           ]
   *         }
   *       }
   *     }
   *   }
   * }
   *
   * Note "missing" is a MongoDB keyword. See https://www.mongodb.com/docs/manual/reference/operator/aggregation/type/
   */

  orion::BSONObjBuilder outer_obj;
  orion::BSONObjBuilder anyElementTrue;
  orion::BSONObjBuilder map_obj;
  orion::BSONObjBuilder in;
  orion::BSONArrayBuilder and_arr;
  orion::BSONObjBuilder and_first;
  orion::BSONObjBuilder and_second;
  orion::BSONObjBuilder and_third;
  orion::BSONObjBuilder regex_obj;
  orion::BSONObjBuilder eq_obj;
  orion::BSONObjBuilder eq_obj_2;
  orion::BSONObjBuilder eq_obj_3;
  orion::BSONObjBuilder eq_obj_4;
  orion::BSONObjBuilder eq_obj_5;
  orion::BSONObjBuilder or_obj;
  orion::BSONObjBuilder type_obj;
  orion::BSONObjBuilder type_obj_1;
  orion::BSONArrayBuilder eq_arr_2;
  orion::BSONArrayBuilder eq_arr_3;
  orion::BSONArrayBuilder eq_arr_4;
  orion::BSONArrayBuilder eq_arr_5;
  orion::BSONArrayBuilder eq_arr_6;
  orion::BSONArrayBuilder or_arr;
  orion::BSONArrayBuilder or_arr_1;

  map_obj.append("input", "$entities");

  regex_obj.append("input", entityId);
  regex_obj.append("regex", "$$this.id");
  regex_obj.append("options", "i");
  and_first.append("$regexMatch", regex_obj.obj());
  and_arr.append(and_first.obj());

  eq_arr_2.append("$$this.type");
  eq_arr_2.append(entityType);
  eq_obj.append("$eq", eq_arr_2.arr());
  or_arr.append(eq_obj.obj());

  eq_arr_3.append("$$this.type");
  eq_arr_3.append("");
  eq_obj_2.append("$eq", eq_arr_3.arr());
  or_arr.append(eq_obj_2.obj());

  type_obj.append("$type", "$$this.type");
  eq_arr_5.append(type_obj.obj());
  eq_arr_5.append("missing");
  eq_obj_3.append("$eq", eq_arr_5.arr());
  or_arr.append(eq_obj_3.obj());

  and_second.append("$or", or_arr.arr());
  and_arr.append(and_second.obj());

  eq_arr_4.append("$$this.isTypePattern");
  eq_arr_4.append(false);
  eq_obj_4.append("$eq", eq_arr_4.arr());
  or_arr_1.append(eq_obj_4.obj());

  type_obj_1.append("$type", "$$this.isTypePattern");
  eq_arr_6.append(type_obj_1.obj());
  eq_arr_6.append("missing");
  eq_obj_5.append("$eq", eq_arr_6.arr());
  or_arr_1.append(eq_obj_5.obj());
  or_obj.append("$or", or_arr_1.arr());

  and_arr.append(or_obj.obj());

  in.append("$and", and_arr.arr());
  map_obj.append("in", in.obj());

  anyElementTrue.append("$map", map_obj.obj());
  outer_obj.append("$anyElementTrue", anyElementTrue.obj());

  orion::BSONObjBuilder bobGtCurrentTime;
  orion::BSONObjBuilder bobNeStatus;

  bobGtCurrentTime.append("$gt", (long long) getCurrentTime());
  bobNeStatus.append("$ne", STATUS_INACTIVE);

  bgP->boPNP.append(CSUB_ENTITIES "." CSUB_ENTITY_ISPATTERN, "true");
  bgP->boPNP.append(CSUB_EXPIRATION, bobGtCurrentTime.obj());
  bgP->boPNP.append(CSUB_STATUS, bobNeStatus.obj());
  bgP->boPNP.append("$expr", outer_obj.obj());
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
  const std::string&  entityType
)
{
  /*
   * Given a subscription document, for each item within the entities array field, the condition is true if:
   *
   * 1. the item isPattern field is "false" AND
   * 2. the item isTypePattern field is true AND
   * 3. the item id field is equal to target id (entityId) AND
   * 4. the item type matches the target entity type (entityType)
   *
   * 3 and 4 are within the same $expr clause.
   *
   * Example of query block generated by this function:
   *
   * {
   *   "entities.isPattern": "false",
   *   "entities.isTypePattern": true,
   *   "expiration": {
   *     "$gt": 1666606572
   *   },
   *   "status": {
   *     "$ne": "inactive"
   *   },
   *   "$expr": {
   *     "$anyElementTrue": {
   *       "$map": {
   *         "input": "$entities",
   *         "in": {
   *           "$and": [
   *             {
   *               "$eq": [
   *                 "$$this.id",
   *                 "(entityId)"
   *               ]
   *             },
   *             {
   *               "$regexMatch": {
   *                 "input": "(entityType)",
   *                 "regex": "$$this.type",
   *                 "options": "i"
   *               }
   *             }
   *           ]
   *         }
   *       }
   *     }
   *   }
   * }
   *
   */

  orion::BSONObjBuilder outer_obj;
  orion::BSONObjBuilder anyElementTrue;
  orion::BSONObjBuilder map_obj;
  orion::BSONObjBuilder in_obj;
  orion::BSONArrayBuilder and_arr;
  orion::BSONObjBuilder and_first;
  orion::BSONObjBuilder and_second;
  orion::BSONObjBuilder regex_obj;
  orion::BSONArrayBuilder eq_arr;

  eq_arr.append("$$this.id");
  eq_arr.append(entityId);
  and_first.append("$eq", eq_arr.arr());
  and_arr.append(and_first.obj());

  regex_obj.append("input", entityType);
  regex_obj.append("regex", "$$this.type");
  regex_obj.append("options", "i");
  and_second.append("$regexMatch", regex_obj.obj());
  and_arr.append(and_second.obj());

  in_obj.append("$and", and_arr.arr());
  map_obj.append("input", "$entities");
  map_obj.append("in", in_obj.obj());

  anyElementTrue.append("$map", map_obj.obj());
  outer_obj.append("$anyElementTrue", anyElementTrue.obj());

  orion::BSONObjBuilder bobGtCurrentTime;
  orion::BSONObjBuilder bobNeStatus;

  bobGtCurrentTime.append("$gt", (long long) getCurrentTime());
  bobNeStatus.append("$ne", STATUS_INACTIVE);

  bgP->boNPP.append(CSUB_ENTITIES "." CSUB_ENTITY_ISPATTERN, "false");
  bgP->boNPP.append(CSUB_ENTITIES "." CSUB_ENTITY_ISTYPEPATTERN, true);
  bgP->boNPP.append(CSUB_EXPIRATION, bobGtCurrentTime.obj());
  bgP->boNPP.append(CSUB_STATUS, bobNeStatus.obj());
  bgP->boNPP.append("$expr", outer_obj.obj());
  bgP->idNPtypeP = bgP->boNPP.obj();
}



/* ****************************************************************************
*
* fill_idPtypeP -
*/
static void fill_idPtypeP
(
  CSubQueryGroup*     bgP,
  const std::string&  entityId,
  const std::string&  entityType
)
{
  /*
   * Given a subscription document, for each item within the entities array field, the condition is true if:
   *
   * 1. the item isPattern field is "true" AND
   * 2. the item isTypePattern field is true AND
   * 3. the item id matches the target entity id (entityId) AND
   * 4. the item type matches the target entity type (entityType)
   *
   * 3 and 4 are within the same $expr clause.
   *
   * Example of query block generated by this function:
   *
   * {
   *   "entities.isPattern": "true",
   *   "entities.isTypePattern": true,
   *   "expiration": {
   *     "$gt": 1666606572
   *   },
   *   "status": {
   *     "$ne": "inactive"
   *   },
   *   "$expr": {
   *     "$anyElementTrue": {
   *       "$map": {
   *         "input": "$entities",
   *         "in": {
   *           "$and": [
   *             {
   *               "$regexMatch": {
   *                 "input": "myEntity",
   *                 "regex": "$$this.id",
   *                 "options": "i"
   *               }
   *             },
   *             {
   *               "$regexMatch": {
   *                 "input": "myType",
   *                 "regex": "$$this.type",
   *                 "options": "i"
   *               }
   *             }
   *           ]
   *         }
   *       }
   *     }
   *   }
   * }
   */

  orion::BSONObjBuilder outer_obj;
  orion::BSONObjBuilder anyElementTrue;
  orion::BSONObjBuilder map_obj;
  orion::BSONObjBuilder in_obj;
  orion::BSONArrayBuilder and_arr;
  orion::BSONObjBuilder and_first;
  orion::BSONObjBuilder and_second;
  orion::BSONObjBuilder regex_obj;
  orion::BSONObjBuilder regex_obj_2;

  regex_obj.append("input", entityId);
  regex_obj.append("regex", "$$this.id");
  regex_obj.append("options", "i");
  and_first.append("$regexMatch", regex_obj.obj());
  and_arr.append(and_first.obj());

  regex_obj_2.append("input", entityType);
  regex_obj_2.append("regex", "$$this.type");
  regex_obj_2.append("options", "i");
  and_second.append("$regexMatch", regex_obj_2.obj());
  and_arr.append(and_second.obj());

  in_obj.append("$and", and_arr.arr());
  map_obj.append("input", "$entities");
  map_obj.append("in", in_obj.obj());

  anyElementTrue.append("$map", map_obj.obj());
  outer_obj.append("$anyElementTrue", anyElementTrue.obj());

  orion::BSONObjBuilder bobGtCurrentTime;
  orion::BSONObjBuilder bobNeStatus;

  bobGtCurrentTime.append("$gt", (long long) getCurrentTime());
  bobNeStatus.append("$ne", STATUS_INACTIVE);

  bgP->boPP.append(CSUB_ENTITIES "." CSUB_ENTITY_ISPATTERN, "true");
  bgP->boPP.append(CSUB_ENTITIES "." CSUB_ENTITY_ISTYPEPATTERN, true);
  bgP->boPP.append(CSUB_EXPIRATION, bobGtCurrentTime.obj());
  bgP->boPP.append(CSUB_STATUS, bobNeStatus.obj());
  bgP->boPP.append("$expr", outer_obj.obj());
  bgP->idPtypeP = bgP->boPP.obj();
}



/* ****************************************************************************
*
* matchAltType
*
*/
static bool matchAltType(orion::BSONObj sub, ngsiv2::SubAltType targetAltType)
{
  std::vector<std::string> altTypeStrings;
  if (sub.hasField(CSUB_ALTTYPES))
  {
    setStringVectorF(sub, CSUB_ALTTYPES, &altTypeStrings);
  }

  // Check targetAltType. If operations field is not there or size == 0 default alterationTypes are update with
  // change and create. Maybe this could be check at MongoDB query stage, but seems be more complex
  if (altTypeStrings.size() == 0)
  {
    if ((isChangeAltType(targetAltType)) || (targetAltType == ngsiv2::SubAltType::EntityCreate))
    {
      return true;
    }
    else
    {
      return false;
    }
  }

  for (unsigned int ix = 0; ix < altTypeStrings.size(); ix++)
  {
    ngsiv2::SubAltType altType = parseAlterationType(altTypeStrings[ix]);
    if (altType == ngsiv2::SubAltType::Unknown)
    {
      LM_E(("Runtime Error (unknown alterationType found in database)"));
    }
    else
    {
      // EntityUpdate is special, it is a "sub-type" of EntityChange
      if (isChangeAltType(targetAltType))
      {
        if ((altType == ngsiv2::SubAltType::EntityUpdate) || (isChangeAltType(altType)))
        {
          return true;
        }
      }
      else if (altType == targetAltType)
      {
        return true;
      }
    }
  }
  return false;
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
  const std::vector<std::string>&                attributes,
  const std::vector<std::string>&                attrsWithModifiedValue,
  const std::vector<std::string>&                attrsWithModifiedMd,
  std::map<std::string, TriggeredSubscription*>& subs,
  std::string&                                   err,
  const std::string&                             tenant,
  const std::vector<std::string>&                servicePathV,
  ngsiv2::SubAltType                             targetAltType
)
{
  std::string               servicePath     = (servicePathV.size() > 0)? servicePathV[0] : "";

  orion::BSONArrayBuilder bab;
  servicePathSubscription(servicePath, &bab);

  /* Build query */

  // Note that, by construction, bab.arr() has always more than one element thus we
  // cannot avoid $in usage
  orion::BSONObjBuilder bobSp;
  bobSp.append("$in", bab.arr());

  /* Query is an $or of 4 sub-clauses:
   *
   * idNPtypeNP -> id no pattern, type no pattern
   * idPtypeNP  -> id pattern,    type no pattern
   * idNPtypeP  -> id no pattern, type pattern
   * idPtypeP   -> id pattern,    type pattern
   */

  //
  // Allocating buffer to hold all these BIG variables, necessary for the population of
  // the four parts of the final query.
  // The necessary variables are too big for the stack and thus moved to the heap, inside CSubQueryGroup.
  //
  CSubQueryGroup* bgP = new CSubQueryGroup();

  // Populating bgP with the four clauses
  fill_idNPtypeNP(bgP, entityId, entityType);
  fill_idPtypeP(bgP,   entityId, entityType);
  fill_idPtypeNP(bgP,  entityId, entityType);
  fill_idNPtypeP(bgP,  entityId, entityType);

  /* Composing final query */
  orion::BSONObjBuilder bobQuery;
  orion::BSONArrayBuilder baFilters;
  baFilters.append(bgP->idNPtypeNP);
  baFilters.append(bgP->idPtypeNP);
  baFilters.append(bgP->idNPtypeP);
  baFilters.append(bgP->idPtypeP);

  bobQuery.append("$or", baFilters.arr());
  bobQuery.append(CSUB_SERVICE_PATH, bobSp.obj());

  bgP->query = bobQuery.obj();

  std::string      db  = composeDatabaseName(tenant);
  orion::DBCursor  cursor;
  std::string      errorString;

  LM_T(LmtMongo, ("query() in '%s' collection: '%s'",
                  (db + '.' + COL_CSUBS).c_str(),
                  bgP->query.toString().c_str()));

  TIME_STAT_MONGO_READ_WAIT_START();
  orion::DBConnection connection = orion::getMongoConnection();

  if (orion::collectionQuery(connection, db, COL_CSUBS, bgP->query, &cursor, &errorString) != true)
  {
    TIME_STAT_MONGO_READ_WAIT_STOP();
    orion::releaseMongoConnection(connection);
    delete bgP;
    return false;
  }
  TIME_STAT_MONGO_READ_WAIT_STOP();

  /* For each one of the subscriptions found, add it to the map (if not already there),
   * after checking triggering attributes */
  orion::BSONObj sub;
  while (cursor.next(&sub))
  {
    orion::BSONElement  idField  = getFieldF(sub, "_id");

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

    std::string subIdStr = idField.OID();

    if (subs.count(subIdStr) == 0)
    {
      // Early extraction of fiedl from DB document. The rest of fields are got later
      bool  notifyOnMetadataChange = sub.hasField(CSUB_NOTIFYONMETADATACHANGE)? getBoolFieldF(sub, CSUB_NOTIFYONMETADATACHANGE) : true;

      // If notifyOnMetadataChange is false and only metadata has been changed, we "downgrade" to ngsiv2::EntityUpdate
      if (!notifyOnMetadataChange && (targetAltType == ngsiv2::EntityChangeOnlyMetadata))
      {
        targetAltType = ngsiv2::EntityUpdate;
      }

      // Check alteration type
      if (!matchAltType(sub, targetAltType))
      {
        continue;
      }

      // Depending of the alteration type, we use the list of attributes in the request or the list
      // with effective modifications. Note that EntityDelete doesn't check the list
      if (targetAltType == ngsiv2::EntityUpdate)
      {
        if (!condValueAttrMatch(sub, attributes))
        {
          continue;
        }
      }
      else if ((isChangeAltType(targetAltType)) || (targetAltType == ngsiv2::EntityCreate))
      {
        // Skip if: 1) there is no change in the *value* of attributes listed in conditions.attrs and 2) there is no change
        // in the *metadata* of the attributes listed in conditions.attrs (the 2) only if notifyOnMetadataChange is true)
        bool b1 = condValueAttrMatch(sub, attrsWithModifiedValue);
        bool b2 = condValueAttrMatch(sub, attrsWithModifiedMd);

        if (!b1 && !(notifyOnMetadataChange && b2))
        {
          continue;
        }
      }

      LM_T(LmtMongo, ("adding subscription: '%s'", sub.toString().c_str()));

      //
      // NOTE: renderFormatString: NGSIv1 JSON is 'default' (for old db-content)
      //
      long long         throttling         = sub.hasField(CSUB_THROTTLING)?       getIntOrLongFieldAsLongF(sub, CSUB_THROTTLING)       : -1;
      long long         maxFailsLimit      = sub.hasField(CSUB_MAXFAILSLIMIT)?    getIntOrLongFieldAsLongF(sub, CSUB_MAXFAILSLIMIT)    : -1;
      long long         failsCounter       = sub.hasField(CSUB_FAILSCOUNTER)?     getIntOrLongFieldAsLongF(sub, CSUB_FAILSCOUNTER)     : -1;
      long long         lastNotification   = sub.hasField(CSUB_LASTNOTIFICATION)? getIntOrLongFieldAsLongF(sub, CSUB_LASTNOTIFICATION) : -1;
      std::string       renderFormatString = sub.hasField(CSUB_FORMAT)? getStringFieldF(sub, CSUB_FORMAT) : "legacy";
      bool              onlyChanged        = sub.hasField(CSUB_ONLYCHANGED)? getBoolFieldF(sub, CSUB_ONLYCHANGED) : false;
      bool              blacklist          = sub.hasField(CSUB_BLACKLIST)? getBoolFieldF(sub, CSUB_BLACKLIST) : false;
      bool              covered            = sub.hasField(CSUB_COVERED)? getBoolFieldF(sub, CSUB_COVERED) : false;
      RenderFormat      renderFormat       = stringToRenderFormat(renderFormatString);
      ngsiv2::HttpInfo  httpInfo;
      ngsiv2::MqttInfo  mqttInfo;

      if (sub.hasField(CSUB_MQTTTOPIC))
      {
        mqttInfo.fill(sub);
      }
      else
      {
        httpInfo.fill(sub);
      }

      StringList aList;
      if (onlyChanged)
      {
        aList = subToAttributeList(sub, blacklist, attributes);
        if (aList.size() == 0)
        {
           continue;
        }
      }
      else
      {
        aList = subToAttributeList(sub);
      }

      TriggeredSubscription* trigs = new TriggeredSubscription
        (
          throttling,
          maxFailsLimit,
          failsCounter,
          lastNotification,
          renderFormat,
          httpInfo,
          mqttInfo,
          aList, "", "", covered);

      // Release after using them to fill the TriggeredSubscription
      httpInfo.release();
      mqttInfo.release();

      if (!onlyChanged)
      {
        trigs->blacklist = blacklist;
      }
      else
      {
        trigs->blacklist = false;
      }

      if (sub.hasField(CSUB_METADATA))
      {
        setStringVectorF(sub, CSUB_METADATA, &(trigs->metadata));
      }

      if (sub.hasField(CSUB_EXPR))
      {
        orion::BSONObj expr = getObjectFieldF(sub, CSUB_EXPR);

        std::string q        = expr.hasField(CSUB_EXPR_Q)      ? getStringFieldF(expr, CSUB_EXPR_Q)      : "";
        std::string mq       = expr.hasField(CSUB_EXPR_MQ)     ? getStringFieldF(expr, CSUB_EXPR_MQ)     : "";
        std::string georel   = expr.hasField(CSUB_EXPR_GEOREL) ? getStringFieldF(expr, CSUB_EXPR_GEOREL) : "";
        std::string geometry = expr.hasField(CSUB_EXPR_GEOM)   ? getStringFieldF(expr, CSUB_EXPR_GEOM)   : "";
        std::string coords   = expr.hasField(CSUB_EXPR_COORDS) ? getStringFieldF(expr, CSUB_EXPR_COORDS) : "";

        trigs->fillExpression(georel, geometry, coords);

        // Parsing q
        if (!q.empty())
        {
          StringFilter* stringFilterP = new StringFilter(SftQ);

          if (stringFilterP->parse(q.c_str(), &err) == false)
          {
            delete stringFilterP;
            delete bgP;

            LM_E(("Runtime Error (%s)", err.c_str()));
            orion::releaseMongoConnection(connection);
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
              orion::releaseMongoConnection(connection);
              return false;
            }

            delete stringFilterP;
          }
        }

        // Parsing mq
        if (!mq.empty())
        {
          StringFilter* mdStringFilterP = new StringFilter(SftMq);

          if (mdStringFilterP->parse(mq.c_str(), &err) == false)
          {
            delete mdStringFilterP;
            delete bgP;

            LM_E(("Runtime Error (%s)", err.c_str()));
            orion::releaseMongoConnection(connection);
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
              orion::releaseMongoConnection(connection);
              return false;
            }

            delete mdStringFilterP;
          }
        }
      }

      subs.insert(std::pair<std::string, TriggeredSubscription*>(subIdStr, trigs));
    }
  }

  orion::releaseMongoConnection(connection);
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
  const std::vector<std::string>&                attributes,
  const std::vector<std::string>&                attrsWithModifiedValues,
  const std::vector<std::string>&                attrsWithModifiedMd,
  std::map<std::string, TriggeredSubscription*>& subs,
  std::string&                                   err,
  std::string                                    tenant,
  const std::vector<std::string>&                servicePathV,
  ngsiv2::SubAltType                             targetAltType
)
{
  extern bool noCache;

  if (noCache)
  {
    return addTriggeredSubscriptions_noCache(entityId, entityType, attributes, attrsWithModifiedValues, attrsWithModifiedMd, subs, err, tenant, servicePathV, targetAltType);
  }
  else
  {
    return addTriggeredSubscriptions_withCache(entityId, entityType, attributes, attrsWithModifiedValues, attrsWithModifiedMd, subs, err, tenant, servicePathV, targetAltType);
  }
}



/* ****************************************************************************
*
* processNotification -
*
* This method returns true if the notification was actually sent. Otherwise, false
* is returned. This is used in the caller to know if lastNotification field in the
* subscription document in csubs collection has to be modified or not.
*/
static bool processNotification
(
  ContextElementResponse*          notifyCerP,
  const StringList&                attrL,
  const std::vector<std::string>&  metadataV,
  std::string                      subId,
  RenderFormat                     renderFormat,
  std::string                      tenant,
  long long                        maxFailsLimit,
  long long                        failsCounter,
  const std::string&               xauthToken,
  const std::string&               fiwareCorrelator,
  unsigned int                     correlatorCounter,
  const ngsiv2::Notification&      notification,
  bool                             blacklist = false,
  bool                             covered = false
)
{
  notifStaticFields nsf;

  nsf.subId             = subId;
  nsf.tenant            = tenant;
  nsf.xauthToken        = xauthToken;
  nsf.fiwareCorrelator  = fiwareCorrelator;
  nsf.correlatorCounter = correlatorCounter;

  getNotifier()->sendNotifyContextRequest(notifyCerP,
                                          notification,
                                          nsf,
                                          maxFailsLimit,
                                          failsCounter,
                                          renderFormat,
                                          attrL.stringV,
                                          blacklist,
                                          covered,
                                          metadataV);

  return true;
}



/* ****************************************************************************
*
* processSubscriptions - send a notification for each subscription in the map
*
* Returns the number of notifications sent as consecuence of the update (used by the
* flow control algorithm)
*/
static unsigned int processSubscriptions
(
  std::map<std::string, TriggeredSubscription*>& subs,
  ContextElementResponse*                        notifyCerP,
  const std::string&                             tenant,
  const std::string&                             xauthToken,
  const std::string&                             fiwareCorrelator,
  unsigned int                                   notifStartCounter
)
{
  // Needed in some calls to underlying logic, although not currently used
  std::string err;

  unsigned int notifSent = 0;

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
    if (tSubP->throttling > 0 && tSubP->lastNotification > -1)
    {
      long long  current               = getCurrentTime();
      long long  sinceLastNotification = current - tSubP->lastNotification;

      if (tSubP->throttling > sinceLastNotification)
      {
        LM_T(LmtMongo, ("blocked due to throttling, current time is: %lld", current));
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
    if ((!tSubP->expression.georel.empty()) && (!tSubP->expression.coords.empty()) && (!tSubP->expression.geometry.empty()))
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

      orion::BSONObjBuilder bobQuery;  // used only to keep processAreaScopeV2() signature
      orion::BSONObjBuilder bobCountQuery;
      if (!processAreaScopeV2(&geoScope, &bobQuery, &bobCountQuery))
      {
        // Error in processAreaScopeV2 is interpreted as no-match (conservative approach)
        continue;
      }

      // Look in the database of an entity that maches the geo-filters. Note that this query doesn't
      // check any other filtering condition, assuming they are already checked in other steps.
      std::string  keyId   = "_id." ENT_ENTITY_ID;
      std::string  keyType = "_id." ENT_ENTITY_TYPE;
      std::string  keySp   = "_id." ENT_SERVICE_PATH;

      std::string  id      = notifyCerP->entity.id;
      std::string  type    = notifyCerP->entity.type;
      std::string  sp      = notifyCerP->entity.servicePath;

      bobCountQuery.append(keyId, id);
      bobCountQuery.append(keyType, type);
      bobCountQuery.append(keySp, sp);

      unsigned long long n;
      if (!orion::collectionCount(composeDatabaseName(tenant), COL_ENTITIES, bobCountQuery.obj(), &n, &filterErr))
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

    // Build notification object. We use topic empty-ness to know the type
    ngsiv2::Notification notification;
    notification.httpInfo.fill(tSubP->httpInfo);
    notification.mqttInfo.fill(tSubP->mqttInfo);
    notification.type = (notification.mqttInfo.topic.empty()? ngsiv2::HttpNotification : ngsiv2::MqttNotification);

    notificationSent = processNotification(notifyCerP,
                                           tSubP->attrL,
                                           tSubP->metadata,
                                           mapSubId,
                                           tSubP->renderFormat,
                                           tenant,
                                           tSubP->maxFailsLimit,
                                           tSubP->failsCounter,
                                           xauthToken,
                                           fiwareCorrelator,
                                           notifStartCounter + notifSent + 1,
                                           notification,
                                           tSubP->blacklist,
                                           tSubP->covered);

    // notification already consumed, it can be freed
    // Only one of the release operations will do something, but it is simpler (and safer)
    // than using notification type
    notification.httpInfo.release();
    notification.mqttInfo.release();

    if (notificationSent)
    {
      notifSent++;

      long long  rightNow  = getCurrentTime();

      orion::BSONObjBuilder bobQuery;
      bobQuery.append("_id", orion::OID(mapSubId));

      orion::BSONObj  query = bobQuery.obj();
      orion::BSONObj  update;

      //
      // If broker running without subscription cache, put lastNotificationTime and count in DB
      //
      if (subCacheActive == false)
      {
        orion::BSONObj  subOrig;
        std::string     newErr;
        std::string     status;

        collectionFindOne(composeDatabaseName(tenant), COL_CSUBS, query, &subOrig, &newErr);
        if (!subOrig.isEmpty())
        {
          if (subOrig.hasField(CSUB_STATUS))
          {
            status = getStringFieldF(subOrig, CSUB_STATUS);
          }
        }

        orion::BSONObjBuilder bobSet;
        bobSet.append(CSUB_LASTNOTIFICATION, rightNow);

        // Update the value of status (in case of oneshot) in DB when broker is running without subscription cache
        if (status == STATUS_ONESHOT)
        {
          bobSet.append(CSUB_STATUS, STATUS_INACTIVE);
        }

        // FIXME #3774: previously this part was based in streamming instead of append()
        orion::BSONObjBuilder bobInc;
        bobInc.append(CSUB_COUNT, (long long) 1);

        orion::BSONObjBuilder bobUpdate;
        bobUpdate.append("$set", bobSet.obj());
        bobUpdate.append("$inc", bobInc.obj());

        orion::collectionUpdate(composeDatabaseName(tenant), COL_CSUBS, query, bobUpdate.obj(), false, &err);
      }


      //
      // Saving lastNotificationTime and count for cached subscription
      //
      if (!tSubP->cacheSubId.empty())
      {
        cacheSemTake(__FUNCTION__, "update lastNotificationTime for cached subscription");

        CachedSubscription*  cSubP = subCacheItemLookup(tSubP->tenant.c_str(), tSubP->cacheSubId.c_str());

        if (cSubP != NULL)
        {
          if (cSubP->status == STATUS_ONESHOT)
          {
            orion::BSONObjBuilder bobSet;
            bobSet.append(CSUB_STATUS, STATUS_INACTIVE);

            orion::BSONObjBuilder bobUpdate;
            bobUpdate.append("$set", bobSet.obj());

            // update the status to inactive as status is oneshot (in both DB and csubs cache)
            orion::collectionUpdate(composeDatabaseName(tenant), COL_CSUBS, query, bobUpdate.obj(), false, &err);
            cSubP->status = STATUS_INACTIVE;
            cSubP->statusLastChange = getCurrentTime();

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

  return notifSent;
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
    Metadata* md    = new Metadata(mdReq);

    caRes->metadataVector.push_back(md);
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
  const std::string&      actionType,
  const bool&             overrideMetadata
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
      if (!targetAttr->type.empty())
      {
        caP->type = targetAttr->type;
      }

      /* Set actionType */
      caP->actionType = actionType;

      /* Set modification date */
      double now = getCurrentTime();
      caP->modDate = now;

      /* Metadata. The metadata previous content is "patched" by the metadata in the request.
       * Exception: a cleanup is done if overrideMetadata option is used and we are not in
       * only value modification case. */
      if ((!targetAttr->onlyValue) && (overrideMetadata))
      {
        caP->metadataVector.release();
      }
      for (unsigned int jx = 0; jx < targetAttr->metadataVector.size(); jx++)
      {
        Metadata* targetMdP = targetAttr->metadataVector[jx];

        /* Search for matching metadata in the CER attribute */
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

            if (!targetMdP->type.empty())
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

  double now = getCurrentTime();
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
* You may wonder why we need toUnset if this function is not related with delete attribute
* logic. However, it's need to "clean" metadata in some cases.
*/
static bool updateContextAttributeItem
(
  ContextElementResponse*   cerP,
  ContextAttribute*         ca,
  orion::BSONObj*           attrsP,
  ContextAttribute*         targetAttr,
  ContextElementResponse*   notifyCerP,
  const std::string&        entityDetail,
  orion::BSONObjBuilder*    toSet,
  orion::BSONObjBuilder*    toUnset,
  orion::BSONArrayBuilder*  attrNamesAdd,
  ChangeType*               changeType,
  bool*                     entityModified,
  std::string*              currentLocAttrName,
  orion::BSONObjBuilder*    geoJson,
  orion::BSONDate*          dateExpiration,
  bool*                     dateExpirationInPayload,
  bool                      isReplace,
  const bool&               forcedUpdate,
  const bool&               overrideMetadata,
  ApiVersion                apiVersion,
  OrionError*               oe
)
{
  std::string err;

  if (updateAttribute(attrsP, toSet, toUnset, attrNamesAdd, targetAttr, changeType, isReplace, forcedUpdate, overrideMetadata, apiVersion))
  {
    // Attribute was found
    *entityModified = *entityModified || (*changeType != NO_CHANGE);
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

      /* Although 'ca' has been already pushed into cerP, the pointer is still valid, of course */
      ca->found = false;
    }
  }
  /* Check aspects related with location and date expiration */
  /* attrP is passed only if existing metadata has to be inspected for ignoreType in geo-location
   * i.e. if overrideMetadata is not in use */
  if (!processLocationAtUpdateAttribute(currentLocAttrName, overrideMetadata? NULL : attrsP, targetAttr, geoJson, &err, apiVersion, oe)
    || !processDateExpirationAtUpdateAttribute(targetAttr, dateExpiration, dateExpirationInPayload, &err, oe))
  {
    std::string details = std::string("action: UPDATE") +
                          " - entity: [" + entityDetail + "]" +
                          " - offending attribute: " + targetAttr->getName() +
                          " - " + err;

    cerP->statusCode.fill(SccInvalidParameter, details);
    // oe->fill() not used, as this is done internally in processLocationAtUpdateAttribute()

    alarmMgr.badInput(clientIp, err, targetAttr->getName());
    return false;
  }

  updateAttrInNotifyCer(notifyCerP, targetAttr, apiVersion == V2, NGSI_MD_ACTIONTYPE_UPDATE, overrideMetadata);

  return true;
}



/* ****************************************************************************
*
* appendContextAttributeItem -
*
* You may wonder why we need toUnset if this function is not related with delete attribute
* logic. However, it's need to "clean" metadata in some cases.
*/
static bool appendContextAttributeItem
(
  ContextElementResponse*   cerP,
  orion::BSONObj*           attrsP,
  ContextAttribute*         targetAttr,
  ContextElementResponse*   notifyCerP,
  const std::string&        entityDetail,
  orion::BSONObjBuilder*    toSet,
  orion::BSONObjBuilder*    toUnset,
  orion::BSONArrayBuilder*  attrNamesAdd,
  ChangeType*               changeType,
  bool*                     entityModified,
  std::string*              currentLocAttrName,
  orion::BSONObjBuilder*    geoJson,
  orion::BSONDate*          dateExpiration,
  const bool&               forcedUpdate,
  const bool&               overrideMetadata,
  ApiVersion                apiVersion,
  OrionError*               oe
)
{
  std::string err;

  bool actualAppend = appendAttribute(attrsP, toSet, toUnset, attrNamesAdd, targetAttr, changeType, forcedUpdate, overrideMetadata, apiVersion);

  *entityModified = *entityModified || (*changeType != NO_CHANGE);

  /* Check aspects related with location */
  /* attrP is passed only if existing metadata has to be inspected for ignoreType in geo-location
   * i.e. if overrideMetadata is not in use */
  if (!processLocationAtAppendAttribute(currentLocAttrName, overrideMetadata? NULL : attrsP, targetAttr, actualAppend, geoJson,
                                        &err, apiVersion, oe)
      || !processDateExpirationAtAppendAttribute(dateExpiration, targetAttr, actualAppend, &err, oe))
  {
    std::string details = std::string("action: APPEND") +
                          " - entity: [" + entityDetail + "]" +
                          " - offending attribute: " + targetAttr->getName() +
                          " - " + err;

    cerP->statusCode.fill(SccInvalidParameter, details);
    // oe->fill() is not used here as it is managed by processLocationAtAppendAttribute()

    alarmMgr.badInput(clientIp, err, targetAttr->getName());
    return false;
  }

  // Note that updateAttrInNotifyCer() may "ruin" targetAttr, as compoundValueP is moved
  // (not copied) to the structure in the notifyCerP and null-ified in targetAttr. Thus, it has
  // to be called after the location processing logic (as this logic may need the compoundValueP

  std::string actionType = (actualAppend == true)? NGSI_MD_ACTIONTYPE_APPEND : NGSI_MD_ACTIONTYPE_UPDATE;
  updateAttrInNotifyCer(notifyCerP, targetAttr, apiVersion == V2, actionType, overrideMetadata);

  return true;
}



/* ****************************************************************************
*
* deleteContextAttributeItem -
*/
static bool deleteContextAttributeItem
(
  ContextElementResponse*   cerP,
  ContextAttribute*         ca,
  orion::BSONObj&           attrs,
  ContextAttribute*         targetAttr,
  ContextElementResponse*   notifyCerP,
  const std::string&        entityDetail,
  orion::BSONObjBuilder*    toUnset,
  orion::BSONArrayBuilder*  attrNamesRemove,
  std::string*              currentLocAttrName,
  bool*                     entityModified,
  orion::BSONDate*          dateExpiration,
  OrionError*               oe
)
{
  if (deleteAttribute(attrs, toUnset, attrNamesRemove, targetAttr))
  {
    deleteAttrInNotifyCer(notifyCerP, targetAttr);
    *entityModified = true;

    /* Check aspects related with location */
    if (targetAttr->getLocation(&attrs))
    {
      std::string details = std::string("action: DELETE") +
                            " - entity: [" + entityDetail + "]" +
                            " - offending attribute: " + targetAttr->getName() +
                            " - location attribute has to be defined at creation time, with APPEND";

      cerP->statusCode.fill(SccInvalidParameter, details);
      oe->fill(SccInvalidModification, details, ERROR_UNPROCESSABLE);

      alarmMgr.badInput(clientIp, "location attribute has to be defined at creation time", targetAttr->getName());
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
      *dateExpiration = orion::BSONDate(NO_EXPIRATION_DATE);
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

    alarmMgr.badInput(clientIp, "attribute to be deleted is not found", targetAttr->getName());
    ca->found = false;
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
  orion::BSONObj&                                 attrs,
  orion::BSONObjBuilder*                          toSet,
  orion::BSONObjBuilder*                          toUnset,
  orion::BSONArrayBuilder*                        attrNamesAdd,
  orion::BSONArrayBuilder*                        attrNamesRemove,
  ContextElementResponse*                         cerP,
  std::string*                                    currentLocAttrName,
  orion::BSONObjBuilder*                          geoJson,
  orion::BSONDate*                                dateExpiration,
  bool*                                           dateExpirationInPayload,
  std::string                                     tenant,
  const std::vector<std::string>&                 servicePathV,
  const bool&                                     forcedUpdate,
  const bool&                                     overrideMetadata,
  ApiVersion                                      apiVersion,
  bool                                            loopDetected,
  OrionError*                                     oe
)
{
  std::string               entityId        = cerP->entity.id;
  std::string               entityType      = cerP->entity.type;
  std::string               entityDetail    = cerP->entity.toString();
  bool                      entityModified  = false;
  std::vector<std::string>  attrsWithModifiedValue;
  std::vector<std::string>  attrsWithModifiedMd;
  std::vector<std::string>  attributes;
  ngsiv2::SubAltType        targetAltType = ngsiv2::SubAltType::EntityUpdate;

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
    ChangeType changeType = CHANGE_VALUE_AND_MD;
    if ((action == ActionTypeUpdate) || (action == ActionTypeReplace))
    {
      if (!updateContextAttributeItem(cerP,
                                      ca,
                                      &attrs,
                                      targetAttr,
                                      notifyCerP,
                                      entityDetail,
                                      toSet,
                                      toUnset,
                                      attrNamesAdd,
                                      &changeType,
                                      &entityModified,
                                      currentLocAttrName,
                                      geoJson,
                                      dateExpiration,
                                      dateExpirationInPayload,
                                      action == ActionTypeReplace,
                                      forcedUpdate,
                                      overrideMetadata,
                                      apiVersion,
                                      oe))
      {
        return false;
      }
    }
    else if ((action == ActionTypeAppend) || (action == ActionTypeAppendStrict))
    {
      if (!appendContextAttributeItem(cerP,
                                      &attrs,
                                      targetAttr,
                                      notifyCerP,
                                      entityDetail,
                                      toSet,
                                      toUnset,
                                      attrNamesAdd,
                                      &changeType,
                                      &entityModified,
                                      currentLocAttrName,
                                      geoJson,
                                      dateExpiration,
                                      forcedUpdate,
                                      overrideMetadata,
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
                                      attrNamesRemove,
                                      currentLocAttrName,
                                      &entityModified,
                                      dateExpiration,
                                      oe))
      {
        return false;
      }
    }
    else
    {
      std::string details = std::string("unknown actionType");

      cerP->statusCode.fill(SccInvalidParameter, details);
      oe->fill(SccBadRequest, details, ERROR_BAD_REQUEST);

      // If we reach this point, there's a BUG in the parse layer checks
      LM_E(("Runtime Error (unknown actionType)"));
      return false;
    }

    /* Add the attribute to the list of attrsWithModifiedValue and attrsWithModifiedMd, in order to
     * check at the end if it triggers some subscription. Note that changeType is always CHANGE_VALUE_AND_MD
     * in the case of  "delete" or "append" */
    if (changeType == CHANGE_VALUE_AND_MD)
    {
      attrsWithModifiedValue.push_back(ca->name);
      attrsWithModifiedMd.push_back(ca->name);
      targetAltType = ngsiv2::SubAltType::EntityChangeBothValueAndMetadata;
    }
    else if (changeType == CHANGE_ONLY_VALUE)
    {
      attrsWithModifiedValue.push_back(ca->name);
      if ((targetAltType == ngsiv2::SubAltType::EntityChangeBothValueAndMetadata) || (targetAltType == ngsiv2::SubAltType::EntityChangeOnlyMetadata))
      {
        targetAltType = ngsiv2::SubAltType::EntityChangeBothValueAndMetadata;
      }
      else
      {
        targetAltType = ngsiv2::SubAltType::EntityChangeOnlyValue;
      }
    }
    else if (changeType == CHANGE_ONLY_MD)
    {
      attrsWithModifiedMd.push_back(ca->name);
      if ((targetAltType == ngsiv2::SubAltType::EntityChangeBothValueAndMetadata) || (targetAltType == ngsiv2::SubAltType::EntityChangeOnlyValue))
      {
        targetAltType = ngsiv2::SubAltType::EntityChangeBothValueAndMetadata;
      }
      else
      {
        targetAltType = ngsiv2::SubAltType::EntityChangeOnlyMetadata;
      }
    }

    attributes.push_back(ca->name);
  }

  /* Add triggered subscriptions */
  std::string err;

  if (loopDetected)
  {
    LM_W(("Notification loop detected for entity id <%s> type <%s>, skipping subscription triggering", entityId.c_str(), entityType.c_str()));
  }
  else if (!addTriggeredSubscriptions(entityId, entityType, attributes, attrsWithModifiedValue, attrsWithModifiedMd, subsToNotify, err, tenant, servicePathV, targetAltType))
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
*/
static bool createEntity
(
  Entity*                          eP,
  const ContextAttributeVector&    attrsV,
  double                           now,
  std::string*                     errDetail,
  std::string                      tenant,
  const std::vector<std::string>&  servicePathV,
  ApiVersion                       apiVersion,
  const std::string&               fiwareCorrelator,
  bool                             upsert,
  OrionError*                      oeP
)
{
  LM_T(LmtMongo, ("Entity not found in '%s' collection, creating it", (composeDatabaseName(tenant) + '.' + COL_ENTITIES).c_str()));

  /* Actually we don't know if this is the first entity (thus, the collection is being created) or not. However, we can
   * invoke ensureLocationIndex() in anycase, given that it is harmless in the case the collection and index already
   * exits (see docs.mongodb.org/manual/reference/method/db.collection.ensureIndex/) */
  ensureLocationIndex(tenant);
  ensureDateExpirationIndex(tenant);

  /* Search for a potential location attribute */
  std::string            locAttr;
  orion::BSONObjBuilder  geoJson;

  if (!processLocationAtEntityCreation(attrsV, &locAttr, &geoJson, errDetail, apiVersion, oeP))
  {
    // oe->fill() already managed by processLocationAtEntityCreation()
    return false;
  }

  /* Search for a potential date expiration attribute */
  orion::BSONDate dateExpiration(NO_EXPIRATION_DATE);

  if (!processDateExpirationAtEntityCreation(attrsV, &dateExpiration, errDetail, oeP))
  {
    // oeP->fill() already managed by processLocationAtEntityCreation()
    return false;
  }

  orion::BSONObjBuilder    attrsToAdd;
  orion::BSONArrayBuilder  attrNamesToAdd;

  for (unsigned int ix = 0; ix < attrsV.size(); ++ix)
  {
    orion::BSONObjBuilder  bsonAttr;

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

    if (!attrsV[ix]->valueBson(std::string(ENT_ATTRS_VALUE), &bsonAttr, attrType, ngsiv1Autocast && (apiVersion == V1)))
    {
      // nothing added to bsonAttr, so attribute is not going to be included in the update to the MongoDB
      // (for example, when $unset:1 is used)
      continue;
    }

    bsonAttr.append(ENT_ATTRS_TYPE, attrType);
    bsonAttr.append(ENT_ATTRS_CREATION_DATE, now);
    bsonAttr.append(ENT_ATTRS_MODIFICATION_DATE, now);

    std::string effectiveName = dbEncode(attrsV[ix]->name);

    LM_T(LmtMongo, ("new attribute: {name: %s, type: %s, value: %s}",
                    effectiveName.c_str(),
                    attrsV[ix]->type.c_str(),
                    attrsV[ix]->getValue().c_str()));

    /* Custom metadata */
    orion::BSONObjBuilder   md;
    orion::BSONArrayBuilder mdNames;
    attrsV[ix]->metadataVector.toBson(&md, &mdNames, apiVersion == V2);
    if (mdNames.arrSize() > 0)
    {
      bsonAttr.append(ENT_ATTRS_MD, md.obj());
    }
    bsonAttr.append(ENT_ATTRS_MDNAMES, mdNames.arr());

    attrsToAdd.append(effectiveName, bsonAttr.obj());
    attrNamesToAdd.append(attrsV[ix]->name);
  }

  orion::BSONObjBuilder bsonIdBuilder;

  bsonIdBuilder.append(ENT_ENTITY_ID, eP->id);

  if (eP->type.empty())
  {
    if (apiVersion == V2)
    {
      // NGSIv2 uses default entity type
      bsonIdBuilder.append(ENT_ENTITY_TYPE, DEFAULT_ENTITY_TYPE);
    }
  }
  else
  {
    bsonIdBuilder.append(ENT_ENTITY_TYPE, eP->type);
  }

  bsonIdBuilder.append(ENT_SERVICE_PATH, servicePathV[0].empty()? SERVICE_PATH_ROOT : servicePathV[0]);

  orion::BSONObj bsonId = bsonIdBuilder.obj();

  orion::BSONObjBuilder insertedDoc;

  insertedDoc.append("_id", bsonId);
  insertedDoc.append(ENT_ATTRNAMES, attrNamesToAdd.arr());
  insertedDoc.append(ENT_ATTRS, attrsToAdd.obj());
  insertedDoc.append(ENT_CREATION_DATE, now);
  insertedDoc.append(ENT_MODIFICATION_DATE, now);

  /* Add location information in the case it was found */
  if (!locAttr.empty())
  {
    orion::BSONObjBuilder bobLocation;
    bobLocation.append(ENT_LOCATION_ATTRNAME, locAttr);
    bobLocation.append(ENT_LOCATION_COORDS, geoJson.obj());

    insertedDoc.append(ENT_LOCATION, bobLocation.obj());
  }

  /* Add date expiration in the case it was found */
  if (!dateExpiration.equal(NO_EXPIRATION_DATE))
  {
    insertedDoc.appendDate(ENT_EXPIRATION, dateExpiration);
  }

  // Correlator (for notification loop detection logic)
  // Note entity creation could be caused by a notification request
  // (with cbnotif= in the correlator) so we need to use correlatorRoot()
  insertedDoc.append(ENT_LAST_CORRELATOR, correlatorRoot(fiwareCorrelator));

  // Why upsert here? See issue: https://github.com/telefonicaid/fiware-orion/issues/3821
  if (upsert)
  {
    orion::BSONObjBuilder q;
    q.append("_id", bsonId);
    if (!orion::collectionUpdate(composeDatabaseName(tenant), COL_ENTITIES, q.obj(), insertedDoc.obj(), true, errDetail))
    {
      // FIXME P7: this checking is weak. If mongoc driver implementation changes, and a slight variation
      // of MONGODB_ERROR_WRONGJSON is used in the error message, then it will break. It would be better to use
      // bson_error_t code (which is numeric) to check this, but this will involved modifications to the
      // orion::collectionInsert() function.
      if (errDetail->find(MONGODB_ERROR_WRONGJSON) != std::string::npos)
      {
        oeP->fill(SccBadRequest, ERROR_DESC_BAD_REQUEST_WRONG_GEOJSON, ERROR_BAD_REQUEST);
      }
      else
      {
        oeP->fill(SccReceiverInternalError, *errDetail, ERROR_INTERNAL_ERROR);
      }
      return false;
    }
  }
  else
  {
    if (!orion::collectionInsert(composeDatabaseName(tenant), COL_ENTITIES, insertedDoc.obj(), errDetail))
    {
      // FIXME P7: same comment in the upsert case
      if (errDetail->find(MONGODB_ERROR_DUPLICATE_KEY) != std::string::npos)
      {
        oeP->fill(SccInvalidModification, ERROR_DESC_UNPROCESSABLE_ALREADY_EXISTS, ERROR_UNPROCESSABLE);
      }
      else if (errDetail->find(MONGODB_ERROR_WRONGJSON) != std::string::npos)
      {
        oeP->fill(SccBadRequest, ERROR_DESC_BAD_REQUEST_WRONG_GEOJSON, ERROR_BAD_REQUEST);
      }
      else
      {
        oeP->fill(SccReceiverInternalError, *errDetail, ERROR_INTERNAL_ERROR);
      }
      return false;
    }
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
  const std::string      idString          = "_id." ENT_ENTITY_ID;
  const std::string      typeString        = "_id." ENT_ENTITY_TYPE;
  const std::string      servicePathString = "_id." ENT_SERVICE_PATH;
  orion::BSONObjBuilder  bob;

  bob.append(idString, entityId);
  if (entityType.empty())
  {
    orion::BSONObjBuilder bobExist;
    bobExist.append("$exists", false);
    bob.append(typeString, bobExist.obj());
  }
  else
  {
    bob.append(typeString, entityType);
  }

  if (servicePath.empty())
  {
    orion::BSONObjBuilder bobExist;
    bobExist.append("$exists", false);
    bob.append(servicePathString, bobExist.obj());
  }
  else
  {
    bob.append(servicePathString, servicePath);
  }

  std::string err;
  if (!collectionRemove(composeDatabaseName(tenant), COL_ENTITIES, bob.obj(), &err))
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
    if (registrationsQuery(enV, attrL, ngsiv2::ForwardUpdate, &crrV, &err, tenant, servicePathV, 0, 0, false))
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
    if (registrationsQuery(enV, attrNullList, ngsiv2::ForwardUpdate, &crrV, &err, tenant, servicePathV, 0, 0, false))
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

      if (!aP->providingApplication.get().empty())
      {
        return true;
      }
    }
  }

  return false;
}



/* ****************************************************************************
*
* calculateOperator -
*
* Return bool if calculated operator has been found (so caller can select to use findAndModify
* instead of usual update)
*/
static bool calculateOperator(ContextElementResponse* cerP, const std::string& op, orion::BSONObjBuilder* b)
{
  bool r = false;

  for (unsigned int ix = 0; ix < cerP->entity.attributeVector.size(); ++ix)
  {
    ContextAttribute* attr = cerP->entity.attributeVector[ix];

    if ((attr->compoundValueP != NULL) && (attr->compoundValueP->childV.size() > 0))
    {
      CompoundValueNode* child0 = attr->compoundValueP->childV[0];
      if ((child0->name == op))
      {
        std::string valueKey = std::string(ENT_ATTRS) + "." + attr->name + "." + ENT_ATTRS_VALUE;
        if (child0->valueType == orion::ValueTypeString)
        {
          b->append(valueKey, child0->stringValue);
        }
        else if (child0->valueType == orion::ValueTypeNumber)
        {
          b->append(valueKey, child0->numberValue);
        }
        else if (child0->valueType == orion::ValueTypeBoolean)
        {
          b->append(valueKey, child0->boolValue);
        }
        else if (child0->valueType == orion::ValueTypeNull)
        {
          b->appendNull(valueKey);
        }
        else if (child0->valueType == orion::ValueTypeVector)
        {
          orion::BSONArrayBuilder ba;
          compoundValueBson(child0->childV, ba, false, false);
          b->append(valueKey, ba.arr());
        }
        else if (child0->valueType == orion::ValueTypeObject)
        {
          orion::BSONObjBuilder bo;
          compoundValueBson(child0->childV, bo, false, false);
          b->append(valueKey, bo.obj());
        }
        else if (child0->valueType == orion::ValueTypeNotGiven)
        {
          LM_E(("Runtime Error (value not given in calculateOperator)"));
          return false;
        }
        else
        {
          LM_E(("Runtime Error (Unknown type in calculateOperator)"));
          return false;
        }

        r = true;
      }
    }
  }

  return r;
}



/* ****************************************************************************
*
* calculateSetOperator -
*
* For objects:
*   {A: {value: {$set: {X: <V1>, Y: <V2>, ...}}} -> {attrs.A.value.X: <V1>, attrs.A.value.Y: <V2>, ... }
*
* For no objects (as regular update):
*   {A: {value: {$set: foo}} -> {attrs.A.value: foo }
*
* Return bool if calculated operator has been found (so caller can select to use findAndModify
* instead of usual update)
*/
static bool calculateSetOperator(ContextElementResponse* cerP, orion::BSONObjBuilder* b)
{
  bool r = false;

  for (unsigned int ix = 0; ix < cerP->entity.attributeVector.size(); ++ix)
  {
    ContextAttribute* attr = cerP->entity.attributeVector[ix];

    if (attr->compoundValueP == NULL)
    {
      continue;
    }

    // Look fo the $set element in the childs (if any)
    int childIx = -1;
    for (unsigned int jx = 0; jx < attr->compoundValueP->childV.size(); ++jx)
    {
      if (attr->compoundValueP->childV[jx]->name == "$set")
      {
        childIx = jx;
        r = true;
      }
    }

    if (childIx == -1)
    {
      continue;
    }

    CompoundValueNode* theChild = attr->compoundValueP->childV[childIx];
    std::string        baseKey  = std::string(ENT_ATTRS) + "." + attr->name + "." + ENT_ATTRS_VALUE;

    // Maybe be needed and we cannot declare it in "case orion::ValueTypeVector" (compilation error)
    orion::BSONArrayBuilder ba;

    switch (theChild->valueType)
    {
    case orion::ValueTypeObject:
      // In this case $set is being used to "edit" an object. Process the
      // childs of theChild
      for (unsigned int jx = 0; jx < theChild->childV.size(); ++jx)
      {
        CompoundValueNode* child = theChild->childV[jx];

        // dbEncode is needed, in order to avoid problems as the one
        // described in issue #4315
        std::string valueKey = baseKey + "." + dbEncode(child->name);

        if (child->valueType == orion::ValueTypeString)
        {
          b->append(valueKey, child->stringValue);
        }
        else if (child->valueType == orion::ValueTypeNumber)
        {
          b->append(valueKey, child->numberValue);
        }
        else if (child->valueType == orion::ValueTypeBoolean)
        {
          b->append(valueKey, child->boolValue);
        }
        else if (child->valueType == orion::ValueTypeNull)
        {
          b->appendNull(valueKey);
        }
        else if (child->valueType == orion::ValueTypeVector)
        {
          orion::BSONArrayBuilder ba;
          compoundValueBson(child->childV, ba, false, false);
          b->append(valueKey, ba.arr());
        }
        else if (child->valueType == orion::ValueTypeObject)
        {
          orion::BSONObjBuilder bo;
          compoundValueBson(child->childV, bo, false, false);
          b->append(valueKey, bo.obj());
        }
        else if (child->valueType == orion::ValueTypeNotGiven)
        {
          LM_E(("Runtime Error (value not given in calculateOperator)"));
        }
        else
        {
          LM_E(("Runtime Error (Unknown type in calculateOperator)"));
        }
      }
      break;

      // From now on, all the cases are $set being use in "replace" mode
      // i.e. as regular update
    case orion::ValueTypeString:
      b->append(baseKey, theChild->stringValue);
      break;

    case orion::ValueTypeNumber:
      b->append(baseKey, theChild->numberValue);
      break;

    case orion::ValueTypeBoolean:
      b->append(baseKey, theChild->boolValue);
      break;

    case orion::ValueTypeNull:
      b->appendNull(baseKey);
      break;

    case orion::ValueTypeVector:
      compoundValueBson(theChild->childV, ba);
      b->append(baseKey, ba.arr());
      break;

    case orion::ValueTypeNotGiven:
      LM_E(("Runtime Error (value not given in calculateOperator)"));
      break;

    default:
      LM_E(("Runtime Error (Unknown type in calculateOperator)"));
      break;
    }
  }

  return r;
}



/* ****************************************************************************
*
* calculateUnsetOperator -
*
* For objects:
*   {A: {value: {$unset: {X: <V1>, Y: <V2>, ...}}} -> {attrs.A.value.X: 1, attrs.A.value.Y: 1, ... }
*
* For no objects (as regular update): ignored
*
* Return bool if calculated operator has been found (so caller can select to use findAndModify
* instead of usual update)
*/
static bool calculateUnsetOperator(ContextElementResponse* cerP, orion::BSONObjBuilder* b)
{
  bool r = false;

  for (unsigned int ix = 0; ix < cerP->entity.attributeVector.size(); ++ix)
  {
    ContextAttribute* attr = cerP->entity.attributeVector[ix];

    if (attr->compoundValueP == NULL)
    {
      continue;
    }

    // Look fo the $unset element in the childs (if any)
    int childIx = -1;
    for (unsigned int jx = 0; jx < attr->compoundValueP->childV.size(); ++jx)
    {
      if (attr->compoundValueP->childV[jx]->name == "$unset")
      {
        childIx = jx;
        r = true;
      }
    }

    if (childIx == -1)
    {
      continue;
    }

    CompoundValueNode* theChild = attr->compoundValueP->childV[childIx];
    std::string        baseKey  = std::string(ENT_ATTRS) + "." + attr->name + "." + ENT_ATTRS_VALUE;

    if (theChild->valueType != orion::ValueTypeObject)
    {
      // Non-object values for $unset are ignored
      continue;
    }

    // Process the childs of theChild
    for (unsigned int jx = 0; jx < theChild->childV.size(); ++jx)
    {
      CompoundValueNode* child = theChild->childV[jx];

      // dbEncode is needed, in order to avoid problems as the one
      // described in issue #4315
      std::string valueKey = baseKey + "." + dbEncode(child->name);
      b->append(valueKey, 1);
    }
  }

  return r;
}



/* ****************************************************************************
*
* updateEntity -
*
* Returns the number of notifications sent as consecuence of the update (used by the
* flow control algorithm)
*/
static unsigned int updateEntity
(
  const orion::BSONObj&           r,
  ActionType                      action,
  const std::string&              tenant,
  const std::vector<std::string>& servicePathV,
  const std::string&              xauthToken,
  Entity*                         eP,
  UpdateContextResponse*          responseP,
  unsigned int*                   attributeAlreadyExistsNumber,
  std::string*                    attributeAlreadyExistsList,
  unsigned int*                   attributeNotExistingNumber,
  std::string*                    attributeNotExistingList,
  const bool&                     forcedUpdate,
  const bool&                     overrideMetadata,
  ApiVersion                      apiVersion,
  const std::string&              fiwareCorrelator,
  unsigned int                    notifStartCounter,
  const std::string&              ngsiV2AttrsFormat
)
{
  // Used to accumulate error response information
  *attributeAlreadyExistsNumber        = 0;
  *attributeAlreadyExistsList          = "[ ";

  *attributeNotExistingNumber          = 0;
  *attributeNotExistingList            = "[ ";

  const std::string  idString          = "_id." ENT_ENTITY_ID;
  const std::string  typeString        = "_id." ENT_ENTITY_TYPE;

  orion::BSONObj            idField           = getObjectFieldF(r, "_id");

  std::string        entityId          = getStringFieldF(idField, ENT_ENTITY_ID);
  std::string        entityType        = idField.hasField(ENT_ENTITY_TYPE) ? getStringFieldF(idField, ENT_ENTITY_TYPE) : "";
  std::string        entitySPath       = getStringFieldF(idField, ENT_SERVICE_PATH);

  EntityId en(entityId, entityType);

  LM_T(LmtServicePath, ("Found entity '%s' in ServicePath '%s'", entityId.c_str(), entitySPath.c_str()));

  ContextElementResponse* cerP = new ContextElementResponse();
  cerP->entity.fill(entityId, entityType, "false");

  /* Build CER used for notifying (if needed) */
  StringList               emptyAttrL;
  ContextElementResponse*  notifyCerP = new ContextElementResponse(r, emptyAttrL);

  // The hasField() check is needed as the entity could have been created with very old Orion version not
  // supporting modification/creation dates
  notifyCerP->entity.creDate = r.hasField(ENT_CREATION_DATE)     ? getNumberFieldF(r, ENT_CREATION_DATE)     : -1;
  notifyCerP->entity.modDate = r.hasField(ENT_MODIFICATION_DATE) ? getNumberFieldF(r, ENT_MODIFICATION_DATE) : -1;

  /* We accumulate the subscriptions in a map. The key of the map is the string representing
   * subscription id */
  std::map<std::string, TriggeredSubscription*> subsToNotify;

  /* If the vector of Context Attributes is empty and the operation was DELETE, then delete the entity */
  if ((action == ActionTypeDelete) && (eP->attributeVector.size() == 0))
  {
    LM_T(LmtServicePath, ("Removing entity"));
    removeEntity(entityId, entityType, cerP, tenant, entitySPath, &(responseP->oe));
    responseP->contextElementResponseVector.push_back(cerP);

    /* EntityDelete subscriptions may be triggered */
    // attrNames is initialized as in addTriggeredSubscriptions() in the create entity logic
    std::string               err;
    std::vector<std::string>  attrNames;
    for (unsigned int ix = 0; ix < eP->attributeVector.size(); ++ix)
    {
      attrNames.push_back(eP->attributeVector[ix]->name);
    }

    // Note we cannot use eP->type for the type, as it may be blank in the request
    // (that would break the cases/1494_subscription_alteration_types/sub_alteration_type_entity_delete2.test case)
    if (!addTriggeredSubscriptions(notifyCerP->entity.id,
                                   notifyCerP->entity.type,
                                   attrNames,
                                   attrNames,
                                   attrNames,
                                   subsToNotify,
                                   err,
                                   tenant,
                                   servicePathV,
                                   ngsiv2::SubAltType::EntityDelete))
    {
      releaseTriggeredSubscriptions(&subsToNotify);
      cerP->statusCode.fill(SccReceiverInternalError, err);
      responseP->oe.fill(SccReceiverInternalError, err, ERROR_INTERNAL_ERROR);

      responseP->contextElementResponseVector.push_back(cerP);
      return 0;  // Error already in responseP
    }

    addBuiltins(notifyCerP, subAltType2string(ngsiv2::SubAltType::EntityDelete));
    unsigned int notifSent = processSubscriptions(subsToNotify,
                                                  notifyCerP,
                                                  tenant,
                                                  xauthToken,
                                                  fiwareCorrelator,
                                                  notifStartCounter);
    releaseTriggeredSubscriptions(&subsToNotify);

    notifyCerP->release();
    delete notifyCerP;

    return notifSent;
  }

  LM_T(LmtServicePath, ("eP->attributeVector.size: %d", eP->attributeVector.size()));
  /* We take as input the attrs array in the entity document and generate two outputs: a
   * BSON object for $set (updates and appends) and a BSON object for $unset (deletes). Note that depending
   * the request one of the BSON objects could be empty (it use to be the $unset one). In addition, for
   * APPEND and DELETE updates we use two arrays to push/pull attributes in the attrsNames vector */

  orion::BSONObj           attrs     = getObjectFieldF(r, ENT_ATTRS);
  orion::BSONObjBuilder    toSet;
  orion::BSONObjBuilder    toUnset;
  orion::BSONArrayBuilder  attrNamesAdd;
  orion::BSONArrayBuilder  attrNamesRemove;

  /* Is the entity using location? In that case, we fill the locAttr and currentGeoJson attributes with that information, otherwise
   * we fill an empty locAttr. Any case, processContextAttributeVector uses that information (and eventually modifies) while it
   * processes the attributes in the updateContext */
  std::string            locAttr = "";
  orion::BSONObj         currentGeoJson;
  orion::BSONObj         newGeoJson;
  orion::BSONObj         finalGeoJson;
  orion::BSONObjBuilder  geoJson;

  if (r.hasField(ENT_LOCATION))
  {
    orion::BSONObj loc    = getObjectFieldF(r, ENT_LOCATION);

    locAttr        = getStringFieldF(loc, ENT_LOCATION_ATTRNAME);
    currentGeoJson = getObjectFieldF(loc, ENT_LOCATION_COORDS);
  }

  /* Is the entity using date expiration? In that case, we fill the currentdateExpiration attribute with that information.
   * In any case, if the request contains a new date expiration, this will become the current one
   * The dateExpirationInPayload boolean is used in case of replace operation,
   * in order to know that the date is a new one, coming from the input request
   */
  orion::BSONDate currentDateExpiration(NO_EXPIRATION_DATE);
  bool dateExpirationInPayload          = false;

  if (r.hasField(ENT_EXPIRATION))
  {
    currentDateExpiration = orion::getField(r, ENT_EXPIRATION).date();
  }

  //
  // Calculate list of existing and not existing attributes (used by decide if this was a partial update, full update or full fail
  // depending on the operation)

  for (unsigned int ix = 0; ix < eP->attributeVector.size(); ++ix)
  {
    if (attrs.hasField(dbEncode(eP->attributeVector[ix]->name)))
    {
      (*attributeAlreadyExistsNumber)++;;

      //
      // If action is ActionTypeAppendStrict, this attribute should now be removed from the 'query' ...
      // processContextAttributeVector looks at the 'skip' field
      //
      if (action == ActionTypeAppendStrict)
      {
        eP->attributeVector[ix]->skip = true;
      }

      // Add to the list of existing attributes
      if (*attributeAlreadyExistsList != "[ ")
      {
        *attributeAlreadyExistsList += ", ";
      }
      *attributeAlreadyExistsList += eP->attributeVector[ix]->name;
    }
    else  // !attrs.hasField(dbEncode(eP->attributeVector[ix]->name))
    {
      (*attributeNotExistingNumber)++;

      // Add to the list of non existing attributes
      if (*attributeNotExistingList != "[ ")
      {
        *attributeNotExistingList += ", ";
      }
      *attributeNotExistingList += eP->attributeVector[ix]->name;
    }
  }
  *attributeNotExistingList += " ]";
  *attributeAlreadyExistsList += " ]";

  // The logic to detect notification loops is to check that the correlator in the request differs from the last one seen for the entity and,
  // in addition, the request was sent due to a custom notification
  bool loopDetected = false;
  if ((ngsiV2AttrsFormat == "custom") && (r.hasField(ENT_LAST_CORRELATOR)))
  {
    loopDetected = (getStringFieldF(r, ENT_LAST_CORRELATOR) == correlatorRoot(fiwareCorrelator));
  }

  if (!processContextAttributeVector(eP,
                                     action,
                                     subsToNotify,
                                     notifyCerP,
                                     attrs,
                                     &toSet,
                                     &toUnset,
                                     &attrNamesAdd,
                                     &attrNamesRemove,
                                     cerP,
                                     &locAttr,
                                     &geoJson,
                                     &currentDateExpiration,
                                     &dateExpirationInPayload,
                                     tenant,
                                     servicePathV,
                                     forcedUpdate,
                                     overrideMetadata,
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

    if (!((*attributeAlreadyExistsNumber) > 0 && (action == ActionTypeAppendStrict)))
    {
      // Note that CER generation in the case of attributeAlreadyExistsNumber has its own logic at
      // processContextElement() function so we need to skip this addition or we will get duplicated
      // CER
      responseP->contextElementResponseVector.push_back(cerP);
    }
    else
    {
      delete cerP;
    }

    /* EntityUpdate subscriptions may be triggered, even in the case of no actual modification */
    addBuiltins(notifyCerP, subAltType2string(ngsiv2::SubAltType::EntityUpdate));
    unsigned int notifSent = processSubscriptions(subsToNotify,
                                                  notifyCerP,
                                                  tenant,
                                                  xauthToken,
                                                  fiwareCorrelator,
                                                  notifStartCounter);
    releaseTriggeredSubscriptions(&subsToNotify);

    notifyCerP->release();
    delete notifyCerP;

    return notifSent;
  }

  /* Compose the final update on database */
  LM_T(LmtServicePath, ("Updating the attributes of the ContextElement"));

  if (action != ActionTypeReplace)
  {
    double now = getCurrentTime();
    toSet.append(ENT_MODIFICATION_DATE, now);
    notifyCerP->entity.modDate = now;
  }

  // We don't touch toSet in the replace case, due to
  // the way in which BSON is composed in that case (see below)
  if (!locAttr.empty())
  {
    newGeoJson = geoJson.obj();

    // If processContextAttributeVector() didn't touched the geoJson, then we
    // use the existing object
    finalGeoJson = newGeoJson.nFields() > 0 ? newGeoJson : currentGeoJson;

    if (action != ActionTypeReplace)
    {
      orion::BSONObjBuilder bobLoc;
      bobLoc.append(ENT_LOCATION_ATTRNAME, locAttr);
      bobLoc.append(ENT_LOCATION_COORDS, finalGeoJson);
      toSet.append(ENT_LOCATION, bobLoc.obj());
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
  if ((!currentDateExpiration.equal(NO_EXPIRATION_DATE)) && (action != ActionTypeReplace))
  {
    toSet.appendDate(ENT_EXPIRATION, currentDateExpiration);
  }
  else if (!dateExpirationInPayload)
  {
    toUnset.append(ENT_EXPIRATION, 1);
  }

  // Correlator (for notification loop detection logic). We don't touch toSet in the replace case, due to
  // the way in which BSON is composed in that case (see below)
  // Note entity update could be caused by a notification request
  // (with cbnotif= in the correlator) so we need to use correlatorRoot()
  if (action != ActionTypeReplace)
  {
    toSet.append(ENT_LAST_CORRELATOR, correlatorRoot(fiwareCorrelator));
  }

  orion::BSONObjBuilder  updatedEntity;
  bool useFindAndModify = false;

  if (action == ActionTypeReplace)
  {
    // toSet: { A1: { ... }, A2: { ... } }
    orion::BSONObjBuilder replaceSet;
    double         now = getCurrentTime();

    // In order to enable easy append management of fields (e.g. location, dateExpiration),
    // we use a BSONObjBuilder instead the BSON stream macro.
    // Note entity replacement could be caused by a notification request
    // (with cbnotif= in the correlator) so we need to use correlatorRoot()
    replaceSet.append(ENT_ATTRS, toSet.obj());
    replaceSet.append(ENT_ATTRNAMES, attrNamesAdd.arr());
    replaceSet.append(ENT_MODIFICATION_DATE, now);
    replaceSet.append(ENT_LAST_CORRELATOR, correlatorRoot(fiwareCorrelator));

    if (dateExpirationInPayload)
    {
      replaceSet.appendDate(ENT_EXPIRATION, currentDateExpiration);
    }

    if (newGeoJson.nFields() > 0)
    {
      orion::BSONObjBuilder bobLoc;
      bobLoc.append(ENT_LOCATION_ATTRNAME, locAttr);
      bobLoc.append(ENT_LOCATION_COORDS, finalGeoJson);
      replaceSet.append(ENT_LOCATION, bobLoc.obj());
    }

    updatedEntity.append("$set", replaceSet.obj());

    if (!dateExpirationInPayload || newGeoJson.nFields() == 0)
    {
      updatedEntity.append("$unset", toUnset.obj());
    }

    notifyCerP->entity.modDate = now;
  }
  else
  {
    // toSet:  { attrs.A1: { ... }, attrs.A2: { ... } }

    // $set is special, as it is shared with regular (without update operator) attribute modification
    bool calculatedSet = calculateSetOperator(notifyCerP, &toSet);
    if (toSet.nFields() > 0)
    {
      updatedEntity.append("$set", toSet.obj());
    }

    // $unset is special, as it is shared with regular (without update operator) attribute modification
    bool calculatedUnset = calculateUnsetOperator(notifyCerP, &toUnset);
    if (toUnset.nFields() > 0)
    {
      updatedEntity.append("$unset", toUnset.obj());
    }

    // $addToSet is special, as it is shared between attrsName additions and
    // attribute operator
    orion::BSONObjBuilder toAddToSet;
    bool calculatedAddToSet = calculateOperator(notifyCerP, "$addToSet", &toAddToSet);
    if (attrNamesAdd.arrSize() > 0)
    {
      orion::BSONObjBuilder bobEach;
      bobEach.append("$each", attrNamesAdd.arr());
      toAddToSet.append(ENT_ATTRNAMES, bobEach.obj());
    }
    if (toAddToSet.nFields() > 0)
    {
      updatedEntity.append("$addToSet", toAddToSet.obj());
    }

    // $pullAll is special, as it is shared between attrsName removals and
    // attribute operator. Probably both cases cannot happen at the same
    // time (as attrsName removal can only happen in DELETE updates), but
    // the logic supports the simultenous case
    orion::BSONObjBuilder toPullAll;
    bool calculatedPullAll = calculateOperator(notifyCerP, "$pullAll", &toPullAll);
    if (attrNamesRemove.arrSize() > 0)
    {
      toPullAll.append(ENT_ATTRNAMES, attrNamesRemove.arr());
    }
    if (toPullAll.nFields() > 0)
    {
      updatedEntity.append("$pullAll", toPullAll.obj());
    }

    // useFindAndModify is set to true if calculation was done
    useFindAndModify = calculatedSet || calculatedUnset || calculatedAddToSet || calculatedPullAll;

    // Note we call calculateOperator() function using notifyCerP instead than eP, given that
    // eP doesn't contain any compound (as they are "stolen" by notifyCerP during the update
    // processing process)
    for (unsigned ix = 0; ix < UPDATE_OPERATORS_NUMBER; ix++)
    {
      orion::BSONObjBuilder b;
      if (calculateOperator(notifyCerP, UPDATE_OPERATORS[ix], &b))
      {
        useFindAndModify = true;
        updatedEntity.append(UPDATE_OPERATORS[ix], b.obj());
      }
    }
  }

  orion::BSONObj updatedEntityObj = updatedEntity.obj();

  /* Note that the query that we build for updating is slighty different than the query used
   * for selecting the entities to process. In particular, the "no type" branch in the if
   * sentence selects precisely the entity with no type, using the {$exists: false} clause */
  orion::BSONObjBuilder query;

  // idString, typeString from earlier in this function
  query.append(idString, entityId);

  if (entityType.empty())
  {
    orion::BSONObjBuilder bob;
    bob.append("$exists", false);
    query.append(typeString, bob.obj());
  }
  else
  {
    query.append(typeString, entityType);
  }

  // Service Path
  if (servicePathFilterNeeded(servicePathV))
  {
    query.appendElements(fillQueryServicePath("_id." ENT_SERVICE_PATH, servicePathV));
  }

  std::string err;
  bool success;
  if (useFindAndModify)
  {
    orion::BSONObj reply;
    success = collectionFindAndModify(composeDatabaseName(tenant), COL_ENTITIES, query.obj(), updatedEntityObj, true, &reply, &err);

    if (success)
    {
      // In success case, update the entity in memory for notifications with the result from DB, as the
      // usage of update operators requires the DB to evaluate the result
      // Note the actual value is not in reply itself, but in the key "value", see
      // https://jira.mongodb.org/browse/CDRIVER-4173
      notifyCerP->release();
      delete notifyCerP;

      notifyCerP = new ContextElementResponse(getObjectFieldF(reply, "value"), emptyAttrL);
    }
  }
  else
  {
    success = collectionUpdate(composeDatabaseName(tenant), COL_ENTITIES, query.obj(), updatedEntityObj, false, &err);
  }
  if (!success)
  {
    cerP->statusCode.fill(SccReceiverInternalError, err);
    responseP->oe.fill(SccReceiverInternalError, err, "InternalServerError");

    responseP->contextElementResponseVector.push_back(cerP);

    releaseTriggeredSubscriptions(&subsToNotify);

    notifyCerP->release();
    delete notifyCerP;

    return 0;
  }

  /* Send notifications for each one of the subscriptions accumulated by
   * previous addTriggeredSubscriptions() invocations. Before that, we add
   * builtin attributes and metadata (both NGSIv1 and NGSIv2 as this is
   * for notifications and NGSIv2 builtins can be used in NGSIv1 notifications) */
  addBuiltins(notifyCerP, subAltType2string(ngsiv2::SubAltType::EntityChangeBothValueAndMetadata));
  unsigned int notifSent = processSubscriptions(subsToNotify,
                                                notifyCerP,
                                                tenant,
                                                xauthToken,
                                                fiwareCorrelator,
                                                notifStartCounter);
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

  return notifSent;
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
        alarmMgr.badInput(clientIp, "duplicated attribute name", name);
        buildGeneralErrorResponse(eP, ca, responseP, SccInvalidModification,
                                  "duplicated attribute /" + name + "/");
        responseP->oe.fill(SccBadRequest, "duplicated attribute /" + name + "/", ERROR_BAD_REQUEST);
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
      if (aP->valueType == orion::ValueTypeNotGiven && aP->type.empty() && (aP->metadataVector.size() == 0))
      {
        ContextAttribute* ca = new ContextAttribute(aP);

        std::string details = std::string("action: ") + actionTypeString(apiVersion, action) +
            " - entity: [" + eP->toString(true) + "]" +
            " - offending attribute: " + aP->name +
            " - empty attribute not allowed in APPEND or UPDATE";

        buildGeneralErrorResponse(eP, ca, responseP, SccInvalidModification, details);
        responseP->oe.fill(SccBadRequest, details, ERROR_BAD_REQUEST);

        alarmMgr.badInput(clientIp, "empty attribute not allowed in APPEND or UPDATE", aP->name);
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
*
* Returns the number of notifications sent as consecuence of the update (used by the
* flow control algorithm)
*/
unsigned int processContextElement
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
  const bool&                          overrideMetadata,
  unsigned int                         notifStartCounter,
  ApiVersion                           apiVersion,
  Ngsiv2Flavour                        ngsiv2Flavour,
  UpdateCoverage*                      updateCoverageP
)
{
  // By default, assume everything is gone to be ok. Next in this function we are going to check for some partial/failure cases
  if (updateCoverageP != NULL)
  {
    *updateCoverageP = UC_SUCCESS;
  }

  /* Check preconditions */
  if (!contextElementPreconditionsCheck(eP, responseP, action, apiVersion))
  {
    return 0;  // Error already in responseP
  }

  /* Find entities (could be several, in the case of no type or isPattern=true) */
  const std::string  idString          = "_id." ENT_ENTITY_ID;
  const std::string  typeString        = "_id." ENT_ENTITY_TYPE;

  orion::BSONObjBuilder  bob;
  EntityId               en(eP->id, eP->type);
  std::string            enStr = eP->id;

  bob.append(idString, eP->id);

  if (!eP->type.empty())
  {
    bob.append(typeString, eP->type);
    enStr += '/' + eP->type;
  }

  // Service path
  if (servicePathFilterNeeded(servicePathV))
  {
    bob.appendElements(fillQueryServicePath("_id." ENT_SERVICE_PATH, servicePathV));
  }

  // FIXME P7: we build the filter for '?!exist=entity::type' directly at mongoBackend layer given that
  // Restriction is not a valid field in updateContext according to the NGSI specification. In the
  // future we may consider to modify the spec to add such Restriction and avoid this ugly "direct injection"
  // of URI filter into mongoBackend
  //
  if (uriParams[URI_PARAM_NOT_EXIST] == SCOPE_VALUE_ENTITY_TYPE)
  {
    std::string  entityTypeString = std::string("_id.") + ENT_ENTITY_TYPE;

    orion::BSONObjBuilder bobExist;
    bobExist.append("$exists", false);

    orion::BSONObjBuilder b;
    b.append(entityTypeString, bobExist.obj());

    bob.appendElements(b.obj());
  }

  orion::BSONObj   query = bob.obj();
  orion::DBCursor  cursor;

  // Several checks related to NGSIv2
  if (apiVersion == V2)
  {
    unsigned long long entitiesNumber;
    std::string        err;

    if (!orion::collectionCount(composeDatabaseName(tenant), COL_ENTITIES, query, &entitiesNumber, &err))
    {
      buildGeneralErrorResponse(eP, NULL, responseP, SccReceiverInternalError, err);
      responseP->oe.fill(SccReceiverInternalError, err, "InternalServerError");
      return 0;
    }

    // This is the case of POST /v2/entities, in order to check that entity doesn't previously exist
    if ((entitiesNumber > 0) && (ngsiv2Flavour == NGSIV2_FLAVOUR_ONCREATE))
    {
      buildGeneralErrorResponse(eP, NULL, responseP, SccInvalidModification, ERROR_DESC_UNPROCESSABLE_ALREADY_EXISTS);
      responseP->oe.fill(SccInvalidModification, ERROR_DESC_UNPROCESSABLE_ALREADY_EXISTS, ERROR_UNPROCESSABLE);
      return 0;
    }

    // This is the case of POST /v2/entities/<id>, in order to check that entity previously exist
    // both for regular case and when ?options=append is used
    if ((entitiesNumber == 0) && (ngsiv2Flavour == NGSIV2_FLAVOUR_ONAPPEND))
    {
      buildGeneralErrorResponse(eP, NULL, responseP, SccContextElementNotFound, ERROR_DESC_NOT_FOUND_ENTITY);
      responseP->oe.fill(SccContextElementNotFound, ERROR_DESC_NOT_FOUND_ENTITY, ERROR_NOT_FOUND);
      return 0;
    }

    // Next block is to avoid that several entities with the same ID get updated at the same time, which is
    // not allowed in NGSIv2. Note that multi-update has been allowed in NGSIv1 (maybe without
    // thinking too much about it, but NGSIv1 behaviour has to be preserved to keep backward compatibility)
    if (entitiesNumber > 1)
    {
      buildGeneralErrorResponse(eP, NULL, responseP, SccConflict, ERROR_DESC_TOO_MANY_ENTITIES);
      responseP->oe.fill(SccConflict, ERROR_DESC_TOO_MANY_ENTITIES, ERROR_TOO_MANY);
      return 0;
    }
  }

  std::string err;

  TIME_STAT_MONGO_READ_WAIT_START();
  orion::DBConnection connection = orion::getMongoConnection();

  if (!orion::collectionQuery(connection, composeDatabaseName(tenant), COL_ENTITIES, query, &cursor, &err))
  {
    orion::releaseMongoConnection(connection);
    TIME_STAT_MONGO_READ_WAIT_STOP();
    buildGeneralErrorResponse(eP, NULL, responseP, SccReceiverInternalError, err);
    responseP->oe.fill(SccReceiverInternalError, err, "InternalServerError");

    return 0;
  }
  TIME_STAT_MONGO_READ_WAIT_STOP();

  //
  // Going through the list of found entities.
  // As ServicePath cannot be modified, inside this loop nothing will be done
  // about ServicePath (The ServicePath was present in the mongo query to obtain the list)
  //
  // FIXME P6: Once we allow for ServicePath to be modified, this loop must be looked at.
  //

  std::vector<orion::BSONObj>  results;
  unsigned int                 docs = 0;

  orion::BSONObj r;
  while (cursor.next(&r))
  {
    docs++;
    LM_T(LmtMongo, ("retrieved document [%d]: '%s'", docs, r.toString().c_str()));

    orion::BSONElement idField = getFieldF(r, "_id");

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

    results.push_back(r);
  }

  orion::releaseMongoConnection(connection);

  LM_T(LmtServicePath, ("Docs found: %d", results.size()));

  unsigned int notifSent = 0;

  // Used to accumulate error response information, checked at the end
  unsigned int  attributeAlreadyExistsNumber = 0;
  std::string   attributeAlreadyExistsList   = "[ ";

  unsigned int  attributeNotExistingNumber  = 0;
  std::string   attributeNotExistingList    = "[ ";

  /* Note that the following loop is not executed if result size is 0, which leads to the
   * 'if' just below to create a new entity */
  for (unsigned int ix = 0; ix < results.size(); ix++)
  {
    notifSent = updateEntity(results[ix],
                             action,
                             tenant,
                             servicePathV,
                             xauthToken,
                             eP,
                             responseP,
                             &attributeAlreadyExistsNumber,
                             &attributeAlreadyExistsList,
                             &attributeNotExistingNumber,
                             &attributeNotExistingList,
                             forcedUpdate,
                             overrideMetadata,
                             apiVersion,
                             fiwareCorrelator,
                             notifStartCounter,
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
          std::string details = ERROR_DESC_DO_NOT_EXIT + enStr + " - [entity itself]";
          responseP->oe.fillOrAppend(SccContextElementNotFound, details, ", " + enStr + " [entity itself]", ERROR_NOT_FOUND);
          if (updateCoverageP != NULL)
          {
            *updateCoverageP = UC_ENTITY_NOT_FOUND;
          }
        }
      }
    }
    else if (action == ActionTypeDelete)
    {
      cerP->statusCode.fill(SccContextElementNotFound);

      std::string details = ERROR_DESC_DO_NOT_EXIT + enStr + " - [entity itself]";
      responseP->oe.fillOrAppend(SccContextElementNotFound, details, ", " + enStr + " [entity itself]", ERROR_NOT_FOUND);
      responseP->contextElementResponseVector.push_back(cerP);
      if (updateCoverageP != NULL)
      {
        *updateCoverageP = UC_ENTITY_NOT_FOUND;
      }
    }
    else   /* APPEND or APPEND_STRICT */
    {
      std::string  errDetail;
      double       now = getCurrentTime();

      // upsert condition is based on ngsiv2Flavour. It happens that when upsert is on,
      // ngsiv2Flavour is NGSIV2_NO_FLAVOUR (see postEntities.cpp code)
      if (!createEntity(eP,
                        eP->attributeVector,
                        now,
                        &errDetail,
                        tenant,
                        servicePathV,
                        apiVersion,
                        fiwareCorrelator,
                        ngsiv2Flavour == NGSIV2_NO_FLAVOUR,
                        &(responseP->oe)))
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
                                       attrNames,
                                       attrNames,
                                       subsToNotify,
                                       err,
                                       tenant,
                                       servicePathV,
                                       ngsiv2::SubAltType::EntityCreate))
        {
          releaseTriggeredSubscriptions(&subsToNotify);
          cerP->statusCode.fill(SccReceiverInternalError, err);
          responseP->oe.fill(SccReceiverInternalError, err, ERROR_INTERNAL_ERROR);

          responseP->contextElementResponseVector.push_back(cerP);
          return 0;  // Error already in responseP
        }

        //
        // Build CER used for notifying (if needed). Service Path vector shouldn't have more than
        // one item, so it should be safe to get item 0
        //
        ContextElementResponse* notifyCerP = new ContextElementResponse(eP, apiVersion == V2);
        notifyCerP->applyUpdateOperators();

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
        addBuiltins(notifyCerP, subAltType2string(ngsiv2::SubAltType::EntityCreate));
        notifSent = processSubscriptions(subsToNotify,
                                         notifyCerP,
                                         tenant,
                                         xauthToken,
                                         fiwareCorrelator,
                                         notifStartCounter);

        notifyCerP->release();
        delete notifyCerP;
        releaseTriggeredSubscriptions(&subsToNotify);
      }

      responseP->contextElementResponseVector.push_back(cerP);
    }
  }

  if ((attributeAlreadyExistsNumber > 0) && (action == ActionTypeAppendStrict))
  {
    std::string details = ERROR_DESC_UNPROCESSABLE_ATTR_ALREADY_EXISTS + enStr + " - " + attributeAlreadyExistsList;
    buildGeneralErrorResponse(eP, NULL, responseP, SccBadRequest, details);
    responseP->oe.fillOrAppend(SccInvalidModification, details, ", " + enStr + " - " + attributeAlreadyExistsList, ERROR_UNPROCESSABLE);
  }

  if ((apiVersion == V2) && (attributeNotExistingNumber > 0) && ((action == ActionTypeUpdate) || (action == ActionTypeDelete)))
  {
    std::string details = ERROR_DESC_DO_NOT_EXIT + enStr + " - " + attributeNotExistingList;
    buildGeneralErrorResponse(eP, NULL, responseP, SccBadRequest, details);
    responseP->oe.fillOrAppend(SccInvalidModification, details, ", " + enStr + " - " + attributeNotExistingList, ERROR_UNPROCESSABLE);
  }

  // The following check makes sense only when there are at least one attribute in the request
  // (e.g. not for entity deletion, which doesn't include attributes)
  if ((updateCoverageP != NULL) && (eP->attributeVector.size() > 0))
  {
    if ((action == ActionTypeUpdate) || (action == ActionTypeDelete))
    {
      // Full failure if the number of the non-existing attributes are all the ones in the request
      if (attributeNotExistingNumber == eP->attributeVector.size())
      {
        *updateCoverageP = UC_FULL_ATTRS_FAIL;
      }
      // Partial failure/success if the number of the non-existing attributes is less than the ones in the request (else branch)
      // and at least there was one existing attribute
      else if (attributeNotExistingNumber > 0)
      {
        *updateCoverageP = UC_PARTIAL;
      }
    }

    if (action == ActionTypeAppendStrict)
    {
      // Full failure if the number of the existing attributes are all the ones in the request
      if (attributeAlreadyExistsNumber == eP->attributeVector.size())
      {
        *updateCoverageP = UC_FULL_ATTRS_FAIL;
      }
      // Partial failure/success if the number of the existing attributes is less than the ones in the request (else branch)
      // and at least there was one non-existing attribute
      else if (attributeAlreadyExistsNumber > 0)
      {
        *updateCoverageP = UC_PARTIAL;
      }
    }
  }

  // Response in responseP
  return notifSent;
}

