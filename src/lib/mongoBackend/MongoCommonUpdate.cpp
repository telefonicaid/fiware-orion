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

#include "mongoBackend/MongoCommonUpdate.h"

#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"

#include "common/globals.h"
#include "common/string.h"
#include "common/sem.h"
#include "mongoBackend/MongoGlobal.h"
#include "mongoBackend/TriggeredSubscription.h"

#include "ngsi/Scope.h"
#include "rest/uriParamNames.h"


using std::string;
using std::map;
using std::auto_ptr;


/* ****************************************************************************
* Forward declarations
*/
static void compoundValueBson(std::vector<orion::CompoundValueNode*> children, BSONObjBuilder& b);


/* ****************************************************************************
*
* compoundValueBson (for arrays) -
*/
static void compoundValueBson(std::vector<orion::CompoundValueNode*> children, BSONArrayBuilder& b)
{
  for (unsigned int ix = 0; ix < children.size(); ++ix)
  {
    orion::CompoundValueNode* child = children[ix];

    if (child->type == orion::CompoundValueNode::String)
    {
      b.append(child->value);
    }
    else if (child->type == orion::CompoundValueNode::Vector)
    {
      BSONArrayBuilder ba;

      compoundValueBson(child->childV, ba);
      b.append(ba.arr());
    }
    else if (child->type == orion::CompoundValueNode::Object)
    {
      BSONObjBuilder bo;

      compoundValueBson(child->childV, bo);
      b.append(bo.obj());
    }
    else
    {
      LM_T(LmtMongo, ("Unknown type in compound value"));
    }
  }
}


/* ****************************************************************************
*
* compoundValueBson -
*/
static void compoundValueBson(std::vector<orion::CompoundValueNode*> children, BSONObjBuilder& b)
{
  for (unsigned int ix = 0; ix < children.size(); ++ix)
  {
    orion::CompoundValueNode* child = children[ix];

    if (child->type == orion::CompoundValueNode::String)
    {
      b.append(child->name, child->value);
    }
    else if (child->type == orion::CompoundValueNode::Vector)
    {
      BSONArrayBuilder ba;

      compoundValueBson(child->childV, ba);
      b.append(child->name, ba.arr());
    }
    else if (child->type == orion::CompoundValueNode::Object)
    {
      BSONObjBuilder bo;

      compoundValueBson(child->childV, bo);
      b.append(child->name, bo.obj());
    }
    else
    {
      LM_T(LmtMongo, ("Unknown type in compound value"));
    }
  }
}


/* ****************************************************************************
*
* valueBson -
*/
static void valueBson(ContextAttribute* ca, BSONObjBuilder& bsonAttr)
{
  if (ca->compoundValueP == NULL)
  {
    bsonAttr.append(ENT_ATTRS_VALUE, ca->value);
  }
  else
  {
    if (ca->compoundValueP->type == orion::CompoundValueNode::Vector)
    {
      BSONArrayBuilder b;
      compoundValueBson(ca->compoundValueP->childV, b);
      bsonAttr.append(ENT_ATTRS_VALUE, b.arr());
    }
    else if (ca->compoundValueP->type == orion::CompoundValueNode::Object)
    {
      BSONObjBuilder b;

      compoundValueBson(ca->compoundValueP->childV, b);
      bsonAttr.append(ENT_ATTRS_VALUE, b.obj());
    }
    else if (ca->compoundValueP->type == orion::CompoundValueNode::String)
    {
      // FIXME P4: this is somehow redundant. See https://github.com/telefonicaid/fiware-orion/issues/271
      bsonAttr.append(ENT_ATTRS_VALUE, ca->compoundValueP->value);
    }
    else
    {
      LM_T(LmtMongo, ("Unknown type in compound value"));
    }
  }
}


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
    Metadata* md = caP->metadataVector.get(ix);

    /* Note that, unlike entity types or attribute types, for attribute metadata we don't consider empty type
     * as a wildcard in order to keep it simpler */
    if ((md->name == name) && (md->type == type))
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
  if (md1.hasField(ENT_ATTRS_MD_TYPE))
  {
    return md2.hasField(ENT_ATTRS_MD_TYPE) &&
      STR_FIELD(md1, ENT_ATTRS_MD_TYPE) == STR_FIELD(md2, ENT_ATTRS_MD_TYPE) &&
      STR_FIELD(md1, ENT_ATTRS_MD_NAME) == STR_FIELD(md2, ENT_ATTRS_MD_NAME);
  }

  return !md2.hasField(ENT_ATTRS_MD_TYPE) &&
    STR_FIELD(md1, ENT_ATTRS_MD_NAME) == STR_FIELD(md2, ENT_ATTRS_MD_NAME);
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
        if (STR_FIELD(md1, ENT_ATTRS_MD_VALUE) != STR_FIELD(md2, ENT_ATTRS_MD_VALUE))
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
* mergeAttrInfo -
*
* Takes as input the information of a given attribute, both in database (attr) and
* request (caP), and merged them producing the mergedAttr output. The function returns
* true if it was an actual update, false otherwise.
*/
static bool mergeAttrInfo(BSONObj& attr, ContextAttribute* caP, BSONObj* mergedAttr)
{
  BSONObjBuilder ab;

  /* 1. Add value */
  valueBson(caP, ab);

  /* Add type, if present in request. If not, just use the one that is already present in the database. */
  if (caP->type != "")
  {
    ab.append(ENT_ATTRS_TYPE, caP->type);
  }
  else
  {
    if (attr.hasField(ENT_ATTRS_TYPE))
    {
      ab.append(ENT_ATTRS_TYPE, STR_FIELD(attr, ENT_ATTRS_TYPE));
    }
  }

  /* 3. Add metadata */
  BSONArrayBuilder mdVBuilder;

  /* First add the metadata elements coming in the request */
  for (unsigned int ix = 0; ix < caP->metadataVector.size() ; ++ix)
  {
    Metadata* md = caP->metadataVector.get(ix);

    /* Skip not custom metadata */
    if (isNotCustomMetadata(md->name))
    {
      continue;
    }
    if (md->type == "")
    {
      mdVBuilder.append(BSON(ENT_ATTRS_MD_NAME << md->name << ENT_ATTRS_MD_VALUE << md->value));
    }
    else
    {
      mdVBuilder.append(BSON(ENT_ATTRS_MD_NAME << md->name <<
                             ENT_ATTRS_MD_TYPE << md->type <<
                             ENT_ATTRS_MD_VALUE << md->value));
    }
  }

  /* Second, for each metadata previously in the metadata vector but *not included in the request*, add it as is */
  int      mdVSize = 0;
  BSONObj  mdV;

  if (attr.hasField(ENT_ATTRS_MD))
  {
    mdV = attr.getField(ENT_ATTRS_MD).embeddedObject();

    for (BSONObj::iterator i = mdV.begin(); i.more();)
    {
      BSONObj md = i.next().embeddedObject();

      mdVSize++;
      if (!hasMetadata(md.getStringField(ENT_ATTRS_MD_NAME), md.getStringField(ENT_ATTRS_MD_TYPE), caP))
      {
        if (md.hasField(ENT_ATTRS_MD_TYPE))
        {
          mdVBuilder.append(BSON(ENT_ATTRS_MD_NAME  << md.getStringField(ENT_ATTRS_MD_NAME) <<
                                 ENT_ATTRS_MD_TYPE  << md.getStringField(ENT_ATTRS_MD_TYPE) <<
                                 ENT_ATTRS_MD_VALUE << md.getStringField(ENT_ATTRS_MD_VALUE)));
        }
        else
        {
          mdVBuilder.append(BSON(ENT_ATTRS_MD_NAME  << md.getStringField(ENT_ATTRS_MD_NAME) <<
                                 ENT_ATTRS_MD_VALUE << md.getStringField(ENT_ATTRS_MD_VALUE)));
        }
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
    ab.append(ENT_ATTRS_CREATION_DATE, attr.getIntField(ENT_ATTRS_CREATION_DATE));
  }

  /* It was an actual update? */
  bool actualUpdate;

  if (caP->compoundValueP == NULL)
  {
    /* In the case of simple value, we consider there is an actual change if one or more of the following are true:
     *
     * 1) the value of the attribute changed (probably the !attr.hasField(ENT_ATTRS_VALUE) is not needed, as any
     *    attribute is supposed to have a value (according to NGSI spec), but it doesn't hurt anyway and makes CB
     *    stronger)
     * 2) the type of the attribute changed (in this case, !attr.hasField(ENT_ATTRS_TYPE) is needed, as attribute
     *    type is optional according to NGSI and the attribute may not have that field in the BSON)
     * 3) the metadata changed (this is done checking if the size of the original and final metadata vectors is
     *    different and, if they are of the same size, checking if the vectors are not equal)
     */
      actualUpdate = (!attr.hasField(ENT_ATTRS_VALUE) ||
                      STR_FIELD(attr, ENT_ATTRS_VALUE) != caP->value ||
                      ((caP->type != "") && (!attr.hasField(ENT_ATTRS_TYPE) ||
                                             STR_FIELD(attr, ENT_ATTRS_TYPE) != caP->type) ) ||
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
      ab.append(ENT_ATTRS_MODIFICATION_DATE, attr.getIntField(ENT_ATTRS_MODIFICATION_DATE));
    }
  }

  *mergedAttr = ab.obj();

  return actualUpdate;
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
*/
static bool updateAttribute(BSONObj& attrs, BSONObjBuilder* toSet, ContextAttribute* caP, bool& actualUpdate)
{
  actualUpdate = false;

  /* Attributes with metadata ID are stored as <attrName>__<ID> in the attributes embedded document */
  std::string effectiveName = dbDotEncode(caP->name);
  if (caP->getId() != "")
  {
    effectiveName += "__" + caP->getId();
  }

  if (!attrs.hasField(effectiveName.c_str()))
  {
    return false;
  }

  BSONObj  attr = attrs.getField(effectiveName).embeddedObject();
  BSONObj  newAttr;

  actualUpdate = mergeAttrInfo(attr, caP, &newAttr);
  if (actualUpdate)
  {
    const std::string composedName = std::string(ENT_ATTRS) + "." + effectiveName;

    toSet->append(composedName, newAttr);
  }

  return true;
}


/* ****************************************************************************
*
* contextAttributeCustomMetadataToBson -
*
* Generates the BSON for metadata vector to be inserted in database for a given atribute.
* If there is no custom metadata, then it returns false (true otherwise).
*
*/
static bool contextAttributeCustomMetadataToBson(BSONObj& mdV, ContextAttribute* ca)
{
  BSONArrayBuilder  mdToAdd;

  for (unsigned int ix = 0; ix < ca->metadataVector.size(); ++ix)
  {
    Metadata* md = ca->metadataVector.get(ix);

    if (!isNotCustomMetadata(md->name))
    {
      mdToAdd.append(BSON("name" << md->name << "type" << md->type << "value" << md->value));
      LM_T(LmtMongo, ("new custom metadata: {name: %s, type: %s, value: %s}",
                      md->name.c_str(), md->type.c_str(), md->value.c_str()));
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
* appendAttribute -
*
* The "actualUpdate" argument (passed by reference) is set to true 1) in the case
* of actual append that, or 2) in the case of append as update if the
* original value of the attribute was different than the one used in the update (this is
* important for ONCHANGE notifications). Otherwise it is false
*
* (Previous versions of this function used the return value for that, but we have
* modified to make in similar to updateAttribute()
*
* Attributes with metadata ID are stored as <attrName>_<ID> in the attributes embedded document
*/
static void appendAttribute(BSONObj& attrs, BSONObjBuilder* toSet, BSONArrayBuilder* toPush, ContextAttribute* caP, bool& actualUpdate)
{
  std::string effectiveName = dbDotEncode(caP->name);

  if (caP->getId() != "")
  {
    effectiveName += "__" + caP->getId();
  }

  /* APPEND with existing attribute equals to UPDATE */
  if (attrs.hasField(effectiveName.c_str()))
  {
    updateAttribute(attrs, toSet, caP, actualUpdate);
    return;
  }

  /* Build the attribute to append */
  BSONObjBuilder ab;

  /* 1. Value */
  valueBson(caP, ab);

  /* 2. Type */
  ab.append(ENT_ATTRS_TYPE, caP->type);

  /* 3. Metadata */
  BSONObj mdV;

  if (contextAttributeCustomMetadataToBson(mdV, caP))
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
static bool legalIdUsage(ContextAttributeVector caV)
{
  for (unsigned int ix = 0; ix < caV.size(); ++ix)
  {
    std::string  attrName  = caV.get(ix)->name;
    std::string  attrType  = caV.get(ix)->type;
    std::string  attrId    = caV.get(ix)->getId();

    if (attrId == "")
    {
      /* Search for attribute with same name and type, but with actual ID to detect inconsistency */
      for (unsigned int jx = 0; jx < caV.size(); ++jx)
      {
        ContextAttribute* ca = caV.get(jx);

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
* processLocation -
*
* This function process the context attribute vector, searching for an attribute marked with
* the location metadata. In that case, it fills locAttr, coordLat and coordLOng. If a location
* attribute is not found, then locAttr is filled with an empty string, i.e. "".
*
* This function always return true (no matter if the attribute was found or not), except in an
* error situation, in which case errorDetail is filled. This can be due to two reasons: ilegal
* usage of the metadata or parsing error in the attribute value.
*
*/
static bool processLocation
(
  ContextAttributeVector  caV,
  std::string&            locAttr,
  double&                 coordLat,
  double&                 coordLong,
  std::string*            errDetail
)
{
  locAttr = "";

  for (unsigned ix = 0; ix < caV.size(); ++ix)
  {
    ContextAttribute* caP = caV.get(ix);

    for (unsigned jx = 0; jx < caP->metadataVector.size(); ++jx)
    {
      Metadata* mdP = caP->metadataVector.get(jx);

      if (mdP->name == NGSI_MD_LOCATION)
      {
        if (locAttr.length() > 0)
        {
          *errDetail = "You cannot use more than one location attribute "
            "when creating an entity [see Orion user manual]";
          return false;
        }
        else
        {
          if ((mdP->value != LOCATION_WGS84) && (mdP->value != LOCATION_WGS84_LEGACY))
          {
            *errDetail = "only WGS84 are supported, found: " + mdP->value;
            return false;
          }

          if (!string2coords(caP->value, coordLat, coordLong))
          {
            *errDetail = "coordinate format error [see Orion user manual]: " + caP->value;
            return false;
          }

          locAttr = caP->name;
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
std::string servicePathSubscriptionRegex(const std::string servicePath, std::vector<std::string>& spathV)
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
* addTriggeredSubscriptions
*
*/
static bool addTriggeredSubscriptions
(
  std::string                               entityId,
  std::string                               entityType,
  std::string                               attr,
  std::map<string, TriggeredSubscription*>& subs,
  std::string&                              err,
  std::string                               tenant,
  const std::vector<std::string>&           servicePathV
)
{
  DBClientBase*             connection      = NULL;
  std::string               servicePath     = (servicePathV.size() > 0)? servicePathV[0] : "";
  std::string               spathRegex      = "";
  std::vector<std::string>  spathV;


  //
  // Create the REGEX for the Service Path
  //
  spathRegex = servicePathSubscriptionRegex(servicePath, spathV);
  spathRegex = std::string("/") + spathRegex + "/";


  /* Build query */
  std::string entIdQ       = CSUB_ENTITIES   "." CSUB_ENTITY_ID;
  std::string entTypeQ     = CSUB_ENTITIES   "." CSUB_ENTITY_TYPE;
  std::string entPatternQ  = CSUB_ENTITIES   "." CSUB_ENTITY_ISPATTERN;
  std::string condTypeQ    = CSUB_CONDITIONS "." CSUB_CONDITIONS_TYPE;
  std::string condValueQ   = CSUB_CONDITIONS "." CSUB_CONDITIONS_VALUE;
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
                condValueQ << attr <<
                CSUB_EXPIRATION   << BSON("$gt" << (long long) getCurrentTime()) <<
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
  queryPattern.append(condValueQ, attr);
  queryPattern.append(CSUB_EXPIRATION, BSON("$gt" << (long long) getCurrentTime()));
  queryPattern.append(CSUB_SERVICE_PATH, spBson);
  queryPattern.appendCode("$where", function);

  // FIXME: condTypeQ, condValueQ and servicePath part could be "factorized" out of the $or clause
  BSONObj query = BSON("$or" << BSON_ARRAY(queryNoPattern << queryPattern.obj()));

  LM_T(LmtMongo, ("query() in '%s' collection: '%s'",
                  getSubscribeContextCollectionName(tenant).c_str(),
                  query.toString().c_str()));

  /* Do the query */
  auto_ptr<DBClientCursor> cursor;
  try
  {
    connection      = getMongoConnection();
    cursor = connection->query(getSubscribeContextCollectionName(tenant).c_str(), query);

    /*
     * We have observed that in some cases of DB errors (e.g. the database daemon is down) instead of
     * raising an exception, the query() method sets the cursor to NULL. In this case, we raise the
     * exception ourselves
     */
    if (cursor.get() == NULL)
    {
      throw DBException("Null cursor from mongo (details on this is found in the source code)", 0);
    }
    releaseMongoConnection(connection);

    LM_I(("Database Operation Successful (%s)", query.toString().c_str()));
  }
  catch (const DBException &e)
  {
    releaseMongoConnection(connection);
    err = std::string("collection: ") + getSubscribeContextCollectionName(tenant).c_str() +
               " - query(): " + query.toString() +
               " - exception: " + e.what();
    LM_E(("Database Error (%s)", err.c_str()));
    return false;
  }
  catch (...)
  {
    releaseMongoConnection(connection);
    err = std::string("collection: ") + getSubscribeContextCollectionName(tenant).c_str() +
               " - query(): " + query.toString() +
               " - exception: " + "generic";
    LM_E(("Database Error (%s)", err.c_str()));
    return false;
  }

  /* For each one of the subscriptions found, add it to the map (if not already there) */
  while (cursor->more())
  {
    BSONObj      sub      = cursor->next();
    BSONElement  idField  = sub.getField("_id");

    //
    // BSONElement::eoo returns true if 'not found', i.e. the field "_id" doesn't exist in 'sub'
    //
    // Now, if 'sub.getField("_id")' is not found, if we continue, calling OID() on it, then we get
    // an exception and the broker crashes.
    //
    if (idField.eoo() == true)
    {
      LM_E(("Database Error (error retrieving _id field in doc: %s)", sub.toString().c_str()));
      continue;
    }

    std::string subIdStr = idField.OID().toString();

    if (subs.count(subIdStr) == 0)
    {
      LM_T(LmtMongo, ("adding subscription: '%s'", sub.toString().c_str()));

      long long throttling       = sub.hasField(CSUB_THROTTLING) ? sub.getField(CSUB_THROTTLING).numberLong() : -1;
      long long lastNotification = sub.hasField(CSUB_LASTNOTIFICATION) ? sub.getIntField(CSUB_LASTNOTIFICATION) : -1;

      TriggeredSubscription* trigs = new TriggeredSubscription
        (
          throttling,
          lastNotification,
          sub.hasField(CSUB_FORMAT) ? stringToFormat(STR_FIELD(sub, CSUB_FORMAT)) : XML,
          STR_FIELD(sub, CSUB_REFERENCE),
          subToAttributeList(sub));

      subs.insert(std::pair<string, TriggeredSubscription*>(subIdStr, trigs));
    }
  }

  return true;
}


/* ****************************************************************************
*
* processSubscriptions
*
*/
static bool processSubscriptions
(
  const EntityId*                           enP,
  std::map<string, TriggeredSubscription*>& subs,
  std::string&                              err,
  std::string                               tenant,
  const std::string&                        xauthToken,
  std::vector<std::string>                  servicePathV
)
{
  DBClientBase* connection = NULL;

  /* For each one of the subscriptions in the map, send notification */
  bool ret = true;
  err = "";

  for (std::map<string, TriggeredSubscription*>::iterator it = subs.begin(); it != subs.end(); ++it)
  {
    std::string             mapSubId  = it->first;
    TriggeredSubscription*  trigs     = it->second;

    if (trigs->throttling != 1 && trigs->lastNotification != 1)
    {
      long long current = getCurrentTime();
      long long sinceLastNotification = current - trigs->lastNotification;

      if (trigs->throttling > sinceLastNotification)
      {
        LM_T(LmtMongo, ("blocked due to throttling, current time is: %l", current));

        trigs->attrL.release();
        delete trigs;

        continue;
      }
    }

    /* Build entities vector */
    EntityIdVector enV;
    enV.push_back(new EntityId(enP->id, enP->type, enP->isPattern));

    /* Send notification */
    if (processOnChangeCondition(enV,
                                 trigs->attrL,
                                 NULL,
                                 mapSubId,
                                 trigs->reference,
                                 trigs->format,
                                 tenant,
                                 xauthToken,
                                 servicePathV))
    {
      BSONObj query = BSON("_id" << OID(mapSubId));
      BSONObj update = BSON("$set" << BSON(CSUB_LASTNOTIFICATION << getCurrentTime()) <<
                            "$inc" << BSON(CSUB_COUNT << 1));

      try
      {
        LM_T(LmtMongo, ("update() in '%s' collection: {%s, %s}", getSubscribeContextCollectionName(tenant).c_str(),
                        query.toString().c_str(),
                        update.toString().c_str()));

        connection = getMongoConnection();
        connection->update(getSubscribeContextCollectionName(tenant).c_str(), query, update);
        releaseMongoConnection(connection);

        LM_I(("Database Operation Successful (update: %s, query: %s)",
              update.toString().c_str(),
              query.toString().c_str()));
      }
      catch (const DBException &e)
      {
        releaseMongoConnection(connection);

        err += std::string("collection: ") + getEntitiesCollectionName(tenant).c_str() +
          " - query(): " + query.toString() + " - update(): " + update.toString() + " - exception: " + e.what();

        LM_E(("Database Error (%s)", err.c_str()));
        ret = false;
      }
      catch (...)
      {
        releaseMongoConnection(connection);

        err += std::string("collection: ") + getEntitiesCollectionName(tenant).c_str() +
          " - query(): " + query.toString() + " - update(): " + update.toString() + " - exception: " + "generic";

        LM_E(("Database Error (%s)", err.c_str()));
        ret = false;
      }
    }

    /* Release object created dynamically (including the value in the map created by addTriggeredSubscriptions */
    trigs->attrL.release();
    enV.release();
    delete it->second;
  }

  subs.clear();
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

  if (caReq->getLocation().length() > 0)
  {
    md = new Metadata(NGSI_MD_LOCATION, "string", caReq->getLocation());
    caRes->metadataVector.push_back(md);
  }

  /* Custom (just "mirroring" in the response) */
  for (unsigned int ix = 0; ix < caReq->metadataVector.size(); ++ix)
  {
    Metadata* mdReq = caReq->metadataVector.get(ix);

    if (!isNotCustomMetadata(mdReq->name))
    {
      md = new Metadata(mdReq->name, mdReq->type, mdReq->value);
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
  BSONObj&                                   attrs,
  BSONObjBuilder*                            toSet,
  BSONObjBuilder*                            toUnset,
  BSONArrayBuilder*                          toPush,
  BSONArrayBuilder*                          toPull,
  ContextElementResponse*                    cerP,
  std::string&                               locAttr,
  double&                                    coordLat,
  double&                                    coordLong,
  std::string                                tenant,
  const std::vector<std::string>&            servicePathV
)
{
  EntityId*                            eP              = &cerP->contextElement.entityId;
  std::string                          entityId        = cerP->contextElement.entityId.id;
  std::string                          entityType      = cerP->contextElement.entityId.type;
  bool                                 entityModified  = false;
  std::map<std::string, unsigned int>  deletedAttributesCounter;  // Aux var for DELETE operations

  for (unsigned int ix = 0; ix < ceP->contextAttributeVector.size(); ++ix)
  {
    ContextAttribute*  targetAttr = ceP->contextAttributeVector.get(ix);

    /* No matter if success or fail, we have to include the attribute in the response */
    ContextAttribute*  ca         = new ContextAttribute(targetAttr->name, targetAttr->type);

    setResponseMetadata(targetAttr, ca);
    cerP->contextElement.contextAttributeVector.push_back(ca);

    /* actualUpdate could be changed to false in the "update" case (or "append as update"). For "delete" and
     * "append" it would keep the true value untouched */
    bool actualUpdate = true;
    if (strcasecmp(action.c_str(), "update") == 0)
    {
      if (updateAttribute(attrs, toSet, targetAttr, actualUpdate))
      {
        entityModified = actualUpdate || entityModified;
      }
      else
      {
        /* If updateAttribute() returns false, then that particular attribute has not
         * been found. In this case, we interrupt the processing and early return with
         * an error StatusCode */
        // FIXME P10: not sure if this .fill() is useless... it seems it is "overriden" by
        // another .fill() in this function caller. We keep it by the moment, but it probably
        // will removed when we refactor this function
        cerP->statusCode.fill(SccInvalidParameter,
                              std::string("action: UPDATE") +
                              " - entity: [" + eP->toString() + "]" +
                              " - offending attribute: " + targetAttr->toString());

        /* Although ca has been already pushed into cerP, it can be used */
        ca->found = false;
      }

      /* Check aspects related with location */
      if (targetAttr->getLocation().length() > 0 && targetAttr->name != locAttr)
      {
        cerP->statusCode.fill(SccInvalidParameter,
                              std::string("action: UPDATE") +
                              " - entity: [" + eP->toString() + "]" +
                              " - offending attribute: " + targetAttr->toString() +
                              " - location nature of an attribute has to be defined at creation time, with APPEND");

        LM_W(("Bad Input (location nature of an attribute has to be defined at creation time, with APPEND)"));
        return false;
      }

      if (locAttr == targetAttr->name)
      {
        if (!string2coords(targetAttr->value, coordLat, coordLong))
        {
          cerP->statusCode.fill(SccInvalidParameter,
                                std::string("action: UPDATE") +
                                " - entity: [" + eP->toString() + "]" +
                                " - offending attribute: " + targetAttr->toString() +
                                " - error parsing location attribute, value: <" + targetAttr->value + ">");

          LM_W(("Bad Input (error parsing location attribute)"));
          return false;
        }
      }
    }
    else if (strcasecmp(action.c_str(), "append") == 0)
    {
      if (legalIdUsage(attrs, targetAttr))
      {
        appendAttribute(attrs, toSet, toPush, targetAttr, actualUpdate);
        entityModified = actualUpdate || entityModified;

        /* Check aspects related with location */
        if (targetAttr->getLocation().length() > 0)
        {
          if (locAttr.length() > 0 && targetAttr->name != locAttr)
          {
            cerP->statusCode.fill(
              SccInvalidParameter,
              std::string("action: APPEND") +
              " - entity: [" + eP->toString() + "]" +
              " - offending attribute: " + targetAttr->toString() +
              " - attempt to define a location attribute [" + targetAttr->name + "]" +
              " when another one has been previously defined [" + locAttr + "]");

            LM_W(("Bad Input (attempt to define a second location attribute)"));
            return false;
          }

          if ((targetAttr->getLocation() != LOCATION_WGS84) && (targetAttr->getLocation() != LOCATION_WGS84_LEGACY))
          {
            cerP->statusCode.fill(
              SccInvalidParameter,
              std::string("action: APPEND") +
              " - entity: [" + eP->toString() + "]" +
              " - offending attribute: " + targetAttr->toString() +
              " - only WGS84 is supported for location, found: [" + targetAttr->getLocation() + "]");

            LM_W(("Bad Input (only WGS84 is supported for location)"));
            return false;
          }

          if (!string2coords(targetAttr->value, coordLat, coordLong))
          {
            cerP->statusCode.fill(SccInvalidParameter,
                                  std::string("action: APPEND") +
                                  " - entity: [" + eP->toString() + "]" +
                                  " - offending attribute: " + targetAttr->toString() +
                                  " - error parsing location attribute, value: [" + targetAttr->value + "]");
            LM_W(("Bad Input (error parsing location attribute)"));
            return false;
          }

          locAttr = targetAttr->name;
        }
      }
      else
      {
        /* If legalIdUsage() returns false, then that particular attribute can not be appended. In this case,
         * we interrupt the processing and early return with
         * a error StatusCode */
        cerP->statusCode.fill(SccInvalidParameter,
                              std::string("action: APPEND") +
                              " - entity: [" + eP->toString() + "]" +
                              " - offending attribute: " + targetAttr->toString() +
                              " - attribute can not be appended");
        LM_W(("Bad Input (attribute can not be appended)"));
        return false;
      }
    }
    else if (strcasecmp(action.c_str(), "delete") == 0)
    {
      if (deleteAttribute(attrs, toUnset, &deletedAttributesCounter, targetAttr))
      {
        entityModified = true;

        /* Check aspects related with location */
        if (targetAttr->getLocation().length() > 0)
        {
          cerP->statusCode.fill(SccInvalidParameter,
                                std::string("action: DELETE") +
                                " - entity: [" + eP->toString() + "]" +
                                " - offending attribute: " + targetAttr->toString() +
                                " - location attribute has to be defined at creation time, with APPEND");

          LM_W(("Bad Input (location attribute has to be defined at creation time)"));
          return false;
        }

        /* Check aspects related with location. "Nullining" locAttr is the way of specifying
         * that location field is no longer used */
        if (locAttr == targetAttr->name)
        {
          locAttr = "";
        }

        ca->found = true;
      }
      else
      {
        /* If deleteAttribute() returns false, then that particular attribute has not
         * been found. In this case, we interrupt the processing and early return with
         * a error StatusCode */
        cerP->statusCode.fill(SccInvalidParameter,
                              std::string("action: DELETE") +
                              " - entity: [" + eP->toString() + "]" +
                              " - offending attribute: " + targetAttr->toString() +
                              " - attribute not found");
        LM_W(("Bad Input (attribute to be deleted is not found)"));
        ca->found = false;

        return false;
      }
    }
    else
    {
      cerP->statusCode.fill(SccInvalidParameter, std::string("unknown actionType: '") + action + "'");

      // This is a BUG in the parse layer checks
      LM_E(("Runtime Error (unknown actionType '%s')", action.c_str()));
      return false;
    }

    /* Add those ONCHANGE subscription triggered by the just processed attribute. Note that
     * actualUpdate is always true in the case of  "delete" or "append", so the if statement
     * is "bypassed" */
    if (actualUpdate)
    {
      std::string err;

      if (!addTriggeredSubscriptions(entityId, entityType, ca->name, subsToNotify, err, tenant, servicePathV))
      {
        cerP->statusCode.fill(SccReceiverInternalError, err);
        return false;
      }
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
  ContextAttributeVector           attrsV,
  std::string*                     errDetail,
  std::string                      tenant,
  const std::vector<std::string>&  servicePathV
)
{
  DBClientBase* connection = NULL;

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

    return false;
  }

  /* Search for a potential location attribute */
  std::string  locAttr;
  double       coordLat;
  double       coordLong;

  if (!processLocation(attrsV, locAttr, coordLat, coordLong, errDetail))
  {
    return false;
  }

  int               now = getCurrentTime();
  BSONObjBuilder    attrsToAdd;
  BSONArrayBuilder  attrNamesToAdd;

  for (unsigned int ix = 0; ix < attrsV.size(); ++ix)
  {
    std::string     attrId = attrsV.get(ix)->getId();
    BSONObjBuilder  bsonAttr;

    bsonAttr.appendElements(BSON(ENT_ATTRS_TYPE << attrsV.get(ix)->type <<
                                 ENT_ATTRS_CREATION_DATE << now <<
                                 ENT_ATTRS_MODIFICATION_DATE << now));

    valueBson(attrsV.get(ix), bsonAttr);

    std::string effectiveName = dbDotEncode(attrsV.get(ix)->name);
    if (attrId.length() != 0)
    {
      effectiveName += "__" + attrId;
    }

    LM_T(LmtMongo, ("new attribute: {name: %s, type: %s, value: %s}",
                    effectiveName.c_str(),
                    attrsV.get(ix)->type.c_str(),
                    attrsV.get(ix)->value.c_str()));

    /* Custom metadata */
    BSONObj mdV;
    if (contextAttributeCustomMetadataToBson(mdV, attrsV.get(ix)))
    {
      bsonAttr.appendArray(ENT_ATTRS_MD, mdV);
    }

    attrsToAdd.append(effectiveName, bsonAttr.obj());
    attrNamesToAdd.append(attrsV.get(ix)->name);
  }

  BSONObj bsonId;

  if (servicePathV.size() == 0)
  {
    LM_T(LmtServicePath, ("Empty service path"));
    bsonId = (eP->type == "")? BSON(ENT_ENTITY_ID << eP->id) :
      BSON(ENT_ENTITY_ID << eP->id << ENT_ENTITY_TYPE << eP->type);
  }
  else
  {
    LM_T(LmtServicePath, ("Service path string: %s", servicePathV[0].c_str()));

    bsonId = (eP->type == "")?
      BSON(ENT_ENTITY_ID << eP->id << ENT_SERVICE_PATH << servicePathV[0]) :
      BSON(ENT_ENTITY_ID << eP->id << ENT_ENTITY_TYPE << eP->type << ENT_SERVICE_PATH << servicePathV[0]);
  }

  BSONObjBuilder insertedDocB;

  insertedDocB.append("_id", bsonId);
  insertedDocB.append(ENT_ATTRNAMES, attrNamesToAdd.arr());
  insertedDocB.append(ENT_ATTRS, attrsToAdd.obj());
  insertedDocB.append(ENT_CREATION_DATE, now);
  insertedDocB.append(ENT_MODIFICATION_DATE, now);

  /* Add location information in the case it was found */
  if (locAttr.length() > 0)
  {
    insertedDocB.append(ENT_LOCATION, BSON(ENT_LOCATION_ATTRNAME << locAttr <<
                                           ENT_LOCATION_COORDS   <<
                                           BSON("type" << "Point" <<
                                                "coordinates" << BSON_ARRAY(coordLong << coordLat))));
  }

  BSONObj insertedDoc = insertedDocB.obj();
  LM_T(LmtMongo, ("insert() in '%s' collection: '%s'",
                  getEntitiesCollectionName(tenant).c_str(),
                  insertedDoc.toString().c_str()));

  try
  {
    connection = getMongoConnection();
    connection->insert(getEntitiesCollectionName(tenant).c_str(), insertedDoc);
    releaseMongoConnection(connection);

    LM_I(("Database Operation Successful (insert %s)", insertedDoc.toString().c_str()));
  }
  catch (const DBException &e)
  {
    releaseMongoConnection(connection);

    *errDetail = std::string("Database Error: collection: ") + getEntitiesCollectionName(tenant).c_str() +
      " - insert(): " + insertedDoc.toString() +
      " - exception: " + e.what();

    LM_E(("Database Error (%s)", errDetail->c_str()));
    return false;
  }
  catch (...)
  {
    releaseMongoConnection(connection);

    *errDetail = std::string("Database Error: collection: ") + getEntitiesCollectionName(tenant).c_str() +
      " - insert(): " + insertedDoc.toString() +
      " - exception: " + "generic";

    LM_E(("Database Error (%s)", errDetail->c_str()));
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
  const std::string&       servicePath
)
{
  const std::string    idString          = "_id." ENT_ENTITY_ID;
  const std::string    typeString        = "_id." ENT_ENTITY_TYPE;
  const std::string    servicePathString = "_id." ENT_SERVICE_PATH;
  DBClientBase*        connection        = NULL;
  BSONObjBuilder       bob;
  BSONObj              query;

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

  query = bob.obj();

  try
  {
    LM_T(LmtMongo, ("remove() in '%s' collection: {%s}", getEntitiesCollectionName(tenant).c_str(),
                    query.toString().c_str()));

    connection = getMongoConnection();
    connection->remove(getEntitiesCollectionName(tenant).c_str(), query);
    releaseMongoConnection(connection);

    LM_I(("Database Operation Successful (remove %s)", query.toString().c_str()));
  }
  catch (const DBException &e)
  {
    releaseMongoConnection(connection);

    cerP->statusCode.fill(SccReceiverInternalError,
                          std::string("collection: ") + getEntitiesCollectionName(tenant).c_str() +
                          " - remove() query: " + query.toString() +
                          " - exception: " + e.what());

    LM_E(("Database Error (%s)", cerP->statusCode.details.c_str()));
    return false;
  }
  catch (...)
  {
    releaseMongoConnection(connection);

    cerP->statusCode.fill(SccReceiverInternalError,
                          std::string("collection: ") + getEntitiesCollectionName(tenant).c_str() +
                          " - remove() query: " + query.toString() +
                          " - exception: " + "generic");

    LM_E(("Database Error (%s)", cerP->statusCode.details.c_str()));
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
    attrL.push_back(caV.get(ix)->name);
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
      LM_E(("Database Error (%s)", err.c_str()));
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
      LM_E(("Database Error (%s)", err.c_str()));
    }
    crrV.release();
  }
}


/* ****************************************************************************
*
* processContextElement -
*
* 0. Preparations
* 1. Preconditions
* 2. Get the complete list of entities from mongo
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
  const std::string&                   caller
)
{
  DBClientBase* connection = NULL;

  /* Getting the entity in the request (helpful in other places) */
  EntityId* enP = &ceP->entityId;

  /* Not supporting isPattern = true currently */
  if (isTrue(enP->isPattern))
  {
    buildGeneralErrorResponse(ceP, NULL, responseP, SccNotImplemented);
    return;
  }

  /* Check that UPDATE or APPEND is not used with attributes with empty value */
  if (strcasecmp(action.c_str(), "update") == 0 || strcasecmp(action.c_str(), "append") == 0)
  {
    for (unsigned int ix = 0; ix < ceP->contextAttributeVector.size(); ++ix)
    {
      ContextAttribute* aP = ceP->contextAttributeVector[ix];

      if ((aP->value.size() == 0) && (aP->compoundValueP == NULL))
      {
        ContextAttribute* ca = new ContextAttribute(aP);

        buildGeneralErrorResponse(ceP, ca, responseP, SccInvalidParameter,
                                  std::string("action: ") + action +
                                  " - entity: [" + enP->toString(true) + "]" +
                                  " - offending attribute: " + aP->toString() +
                                  " - empty attribute not allowed in APPEND or UPDATE");
        LM_W(("Bad Input (empty attribute not allowed in APPEND or UPDATE)"));
        return;
      }
    }
  }

  /* Find entities (could be several, in the case of no type or isPattern=true) */
  const std::string  idString          = "_id." ENT_ENTITY_ID;
  const std::string  typeString        = "_id." ENT_ENTITY_TYPE;
  const std::string  servicePathString = "_id." ENT_SERVICE_PATH;
  BSONObjBuilder     bob;

  bob.append(idString, enP->id);

  if (enP->type != "")
  {
    bob.append(typeString, enP->type);
  }

  if (servicePathV.size() == 0)
  {
    bob.append(servicePathString, BSON("$exists" << false));
    LM_T(LmtServicePath, ("Updating entity '%s' (no Service Path), action '%s'",
                          ceP->entityId.id.c_str(),
                          action.c_str()));
  }
  else
  {
    LM_T(LmtServicePath, ("Updating entity '%s' for Service Path: '%s', action '%s'",
                          ceP->entityId.id.c_str(),
                          servicePathV[0].c_str(),
                          action.c_str()));

    char               path[MAX_SERVICE_NAME_LEN];
    slashEscape(servicePathV[0].c_str(), path, sizeof(path));

    const std::string  servicePathValue  = std::string("^") + path + "$";
    bob.appendRegex(servicePathString, servicePathValue);
  }


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

  try
  {
    LM_T(LmtMongo, ("query() in '%s' collection: '%s'",
                    getEntitiesCollectionName(tenant).c_str(),
                    query.toString().c_str()));

    connection = getMongoConnection();
    cursor     = connection->query(getEntitiesCollectionName(tenant).c_str(), query);

    /*
     * We have observed that in some cases of DB errors (e.g. the database daemon is down) instead of
     * raising an exception, the query() method sets the cursor to NULL. In this case, we raise the
     * exception ourselves
     */
    if (cursor.get() == NULL)
    {
      throw DBException("Null cursor from mongo (details on this is found in the source code)", 0);
    }
    releaseMongoConnection(connection);

    LM_I(("Database Operation Successful (%s)", query.toString().c_str()));
  }
  catch (const DBException &e)
  {
    releaseMongoConnection(connection);

    buildGeneralErrorResponse(ceP, NULL, responseP, SccReceiverInternalError,
                              std::string("collection: ") + getEntitiesCollectionName(tenant).c_str() +
                              " - query(): " + query.toString() +
                              " - exception: " + e.what());
    LM_E(("Database Error ('%s', '%s')", query.toString().c_str(), e.what()));
    return;
  }
  catch (...)
  {
    releaseMongoConnection(connection);

    buildGeneralErrorResponse(ceP, NULL, responseP, SccReceiverInternalError,
                              std::string("collection: ") + getEntitiesCollectionName(tenant).c_str() +
                              " - query(): " + query.toString() +
                              " - exception: " + "generic");
    LM_E(("Database Error ('%s', '%s')", query.toString().c_str(), "generic exception"));
    return;
  }


  //
  // Going through the list of found entities.
  // As ServicePath cannot be modified, inside this loop nothing will be done
  // about ServicePath (The ServicePath was present in the mongo query to obtain the list)
  //
  // FIXME P6: Once we allow for ServicePath to be modified, this loop must be looked at.
  //
  int docs = 0;

  while (cursor->more())
  {
    BSONObj r = cursor->next();

    LM_T(LmtMongo, ("retrieved document: '%s'", r.toString().c_str()));
    ++docs;

    BSONElement idField = r.getField("_id");

    //
    // BSONElement::eoo returns true if 'not found', i.e. the field "_id" doesn't exist in 'sub'
    //
    // Now, if 'r.getField("_id")' is not found, if we continue, calling embeddedObject() on it, then we get
    // an exception and the broker crashes.
    //
    if (idField.eoo() == true)
    {
      LM_E(("Database Error (error retrieving _id field in doc: %s)", r.toString().c_str()));
      continue;
    }

    std::string entityId    = STR_FIELD(idField.embeddedObject(), ENT_ENTITY_ID);
    std::string entityType  = STR_FIELD(idField.embeddedObject(), ENT_ENTITY_TYPE);
    std::string entitySPath = STR_FIELD(idField.embeddedObject(), ENT_SERVICE_PATH);

    LM_T(LmtServicePath, ("Found entity '%s' in ServicePath '%s'", entityId.c_str(), entitySPath.c_str()));

    ContextElementResponse* cerP = new ContextElementResponse();
    cerP->contextElement.entityId.fill(entityId, entityType, "false");

    /* If the vector of Context Attributes is empty and the operation was DELETE, then delete the entity */
    if (strcasecmp(action.c_str(), "delete") == 0 && ceP->contextAttributeVector.size() == 0) {
      LM_T(LmtServicePath, ("Removing entity"));
      removeEntity(entityId, entityType, cerP, tenant, entitySPath);
      responseP->contextElementResponseVector.push_back(cerP);
      continue;
    }

    LM_T(LmtServicePath, ("ceP->contextAttributeVector.size: %d", ceP->contextAttributeVector.size()));
    /* We take as input the attrs array in the entity document and generate two outputs: a
     * BSON object for $set (updates and appends) and a BSON object for $unset (deletes). Note that depending
     * the request one of the BSON objects could be empty (it use to be the $unset one). In addition, for
     * APPEND and DELETE updates we use two arrays to push/pull attributes in the attrsNames vector */
    BSONObj           attrs = r.getField(ENT_ATTRS).embeddedObject();
    BSONObjBuilder    toSet;
    BSONObjBuilder    toUnset;
    BSONArrayBuilder  toPush;
    BSONArrayBuilder  toPull;

    /* We accumulate the subscriptions in a map. The key of the map is the string representing
     * subscription id */
    std::map<string, TriggeredSubscription*> subsToNotify;

    /* Is the entity using location? In that case, we fill the locAttr, coordLat and coordLong attributes with that information, otherwise
     * we fill an empty locAttrs. Any case, processContextAttributeVector uses that information (and eventually modifies) while it
     * processes the attributes in the updateContext */
    std::string  locAttr = "";
    double       coordLat;
    double       coordLong;

    if (r.hasField(ENT_LOCATION))
    {
      //
      // FIXME P2: potentially, assertion error will happen if the field is not as expected.
      //           Although this shouldn't happen (if it happens, it means that somebody has manipulated the
      //           DB out-of-band of the context broker), a safer way of parsing BSON object
      //           will be needed. This is a general comment, applicable to many places in the mongoBackend code
      //
      BSONObj loc = r.getObjectField(ENT_LOCATION);

      locAttr     = loc.getStringField(ENT_LOCATION_ATTRNAME);
      coordLong   = loc.getObjectField(ENT_LOCATION_COORDS).getField("coordinates").Array()[0].Double();
      coordLat    = loc.getObjectField(ENT_LOCATION_COORDS).getField("coordinates").Array()[1].Double();
    }

    if (!processContextAttributeVector(ceP,
                                       action,
                                       subsToNotify,
                                       attrs,
                                       &toSet,
                                       &toUnset,
                                       &toPush,
                                       &toPull,
                                       cerP,
                                       locAttr,
                                       coordLat,
                                       coordLong,
                                       tenant,
                                       servicePathV))
    {
      /* The entity wasn't actually modified, so we don't need to update it and we can continue with next one */
      // FIXME P8: the same three statements are at the end of the while loop. Refactor the code to have this
      // in only one place
      searchContextProviders(tenant, servicePathV, *enP, ceP->contextAttributeVector, cerP);
      responseP->contextElementResponseVector.push_back(cerP);
      releaseTriggeredSubscriptions(subsToNotify);
      continue;
    }

    /* Compose the final update on database */
    LM_T(LmtServicePath, ("Updating the attributes of the ContextElement"));

    toSet.append(ENT_MODIFICATION_DATE, getCurrentTime());
    if (locAttr.length() > 0)
    {
      toSet.append(ENT_LOCATION, BSON(ENT_LOCATION_ATTRNAME << locAttr <<
                                      ENT_LOCATION_COORDS   <<
                                      BSON("type" << "Point" <<
                                           "coordinates" << BSON_ARRAY(coordLong << coordLat))));
    }
    else
    {
      toUnset.append(ENT_LOCATION, 1);
    }

    /* FIXME: I don't like the obj() step, but it could be the only possible way, let's wait for the answer to
     * http://stackoverflow.com/questions/29668439/get-number-of-fields-in-bsonobjbuilder-object */
    BSONObjBuilder  updatedEntity;
    BSONObj         toSetObj    = toSet.obj();
    BSONObj         toUnsetObj  = toUnset.obj();
    BSONArray       toPushArr   = toPush.arr();
    BSONArray       toPullArr   = toPull.arr();

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

    BSONObj updatedEntityObj = updatedEntity.obj();

    /* Note that the query that we build for updating is slighty different than the query used
     * for selecting the entities to process. In particular, the "no type" branch in the if
     * sentence selects precisely the entity with no type, using the {$exists: false} clause */
    BSONObjBuilder bob;

    // idString, typeString from earlier in this function
    bob.append(idString, entityId);

    if (entityType == "")
    {
      bob.append(typeString, BSON("$exists" << false));
    }
    else
    {
      bob.append(typeString, entityType);
    }

    // The servicePath of THIS object is entitySPath
    char espath[MAX_SERVICE_NAME_LEN];
    slashEscape(entitySPath.c_str(), espath, sizeof(espath));

    // servicePathString from earlier in this function
    if (servicePathV.size() == 0)
    {
      bob.append(servicePathString, BSON("$exists" << false));
    }
    else
    {
      bob.appendRegex(servicePathString, espath);
    }

    BSONObj query = bob.obj();

    try
    {
      LM_T(LmtMongo, ("update() in '%s' collection: {%s, %s}", getEntitiesCollectionName(tenant).c_str(),
                      query.toString().c_str(),
                      updatedEntityObj.toString().c_str()));

      connection = getMongoConnection();
      connection->update(getEntitiesCollectionName(tenant).c_str(), query, updatedEntityObj);
      releaseMongoConnection(connection);

      LM_I(("Database Operation Successful (update %s)", query.toString().c_str()));
    }
    catch (const DBException &e)
    {
      releaseMongoConnection(connection);

      cerP->statusCode.fill(SccReceiverInternalError,
                            std::string("collection: ") + getEntitiesCollectionName(tenant).c_str() +
                            " - update() query: " + query.toString() +
                            " - update() doc: " + updatedEntityObj.toString() +
                            " - exception: " + e.what());

      responseP->contextElementResponseVector.push_back(cerP);
      LM_E(("Database Error (%s)", cerP->statusCode.details.c_str()));
      releaseTriggeredSubscriptions(subsToNotify);
      continue;
    }
    catch (...)
    {
      releaseMongoConnection(connection);

      cerP->statusCode.fill(SccReceiverInternalError,
                            std::string("collection: ") + getEntitiesCollectionName(tenant).c_str() +
                            " - update() query: " + query.toString() +
                            " - update() doc: " + updatedEntityObj.toString() +
                            " - exception: " + "generic");

      responseP->contextElementResponseVector.push_back(cerP);
      LM_E(("Database Error (%s)", cerP->statusCode.details.c_str()));
      releaseTriggeredSubscriptions(subsToNotify);
      continue;
    }

    /* Send notifications for each one of the ONCHANGE subscriptions accumulated by
     * previous addTriggeredSubscriptions() invocations */
    std::string err;
    processSubscriptions(enP, subsToNotify, err, tenant, xauthToken, servicePathV);

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
  LM_T(LmtServicePath, ("Docs found: %d", docs));

  /*
   * If the entity doesn't already exist, we create it. Note that alternatively, we could do a count()
   * before the query() to check this. However this would add a second interaction with MongoDB.
   *
   * Here we set the ServicePath if set in the request (if APPEND, of course).
   * Actually, the 'slash-escaped' ServicePath (variable: 'path') is sent to the function createEntity
   * which sets the ServicePath for the entity.
   */
  if (docs == 0)
  {
    /* Creating the common par of the response that doesn't depend on the case */
    ContextElementResponse* cerP = new ContextElementResponse();

    cerP->contextElement.entityId.fill(enP->id, enP->type, "false");

    /* All the attributes existing in the request are added to the response with 'found' set to false
     * in the of UPDATE/DELETE and true in the case of APPEND
     */
    bool foundValue = (strcasecmp(action.c_str(), "append") == 0);

    for (unsigned int ix = 0; ix < ceP->contextAttributeVector.size(); ++ix)
    {
      ContextAttribute*  caP  = ceP->contextAttributeVector.get(ix);
      ContextAttribute*  ca   = new ContextAttribute(caP->name, caP->type, "", foundValue);

      setResponseMetadata(caP, ca);
      cerP->contextElement.contextAttributeVector.push_back(ca);
    }

    if (strcasecmp(action.c_str(), "update") == 0)
    {
      /* In the case of UPDATE or DELETE we look for context providers */
      searchContextProviders(tenant, servicePathV, *enP, ceP->contextAttributeVector, cerP);
      cerP->statusCode.fill(SccOk);
      responseP->contextElementResponseVector.push_back(cerP);
    }
    else if (strcasecmp(action.c_str(), "delete") == 0)
    {
      cerP->statusCode.fill(SccContextElementNotFound);
      responseP->contextElementResponseVector.push_back(cerP);
    }
    else   /* APPEND */
    {
      std::string errReason, errDetail;

      if (!createEntity(enP, ceP->contextAttributeVector, &errDetail, tenant, servicePathV))
      {
        cerP->statusCode.fill(SccInvalidParameter, errDetail);
      }
      else
      {
        cerP->statusCode.fill(SccOk);

        /* Successful creation: send potential notifications */
        std::map<string, TriggeredSubscription*> subsToNotify;

        for (unsigned int ix = 0; ix < ceP->contextAttributeVector.size(); ++ix)
        {
          std::string err;

          if (!addTriggeredSubscriptions(enP->id,
                                         enP->type,
                                         ceP->contextAttributeVector.get(ix)->name,
                                         subsToNotify,
                                         err,
                                         tenant,
                                         servicePathV))
          {
            cerP->statusCode.fill(SccReceiverInternalError, err);
            responseP->contextElementResponseVector.push_back(cerP);
            return;
          }
        }

        processSubscriptions(enP, subsToNotify, errReason, tenant, xauthToken, servicePathV);
      }

      responseP->contextElementResponseVector.push_back(cerP);
    }
  }
}
