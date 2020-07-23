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
#include <string>

#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"

#include "common/tag.h"
#include "common/RenderFormat.h"
#include "alarmMgr/alarmMgr.h"
#include "ngsi/ContextElementResponse.h"
#include "ngsi/StringList.h"
#include "ngsi10/QueryContextResponse.h"

#include "mongoBackend/dbConstants.h"
#include "mongoBackend/dbFieldEncoding.h"
#include "mongoBackend/compoundResponses.h"
#include "mongoBackend/MongoGlobal.h"       // includedAttribute

#include "mongoDriver/safeMongo.h"



/* ****************************************************************************
*
* ContextElementResponse::ContextElementResponse -
*/
ContextElementResponse::ContextElementResponse()
{
  prune = false;
}



/* ****************************************************************************
*
* ContextElementResponse::ContextElementResponse - 
*/
ContextElementResponse::ContextElementResponse(EntityId* eP, ContextAttribute* aP)
{
  prune = false;

  entity.fill(eP->id, eP->type, eP->isPattern);

  if (aP != NULL)
  {
    entity.attributeVector.push_back(new ContextAttribute(aP));
  }
}



/* ****************************************************************************
*
* ContextElementResponse::ContextElementResponse - 
*/
ContextElementResponse::ContextElementResponse(ContextElementResponse* cerP, bool cloneCompounds)
{
  prune = false;

  entity.fill(cerP->entity, false, cloneCompounds);
  statusCode.fill(cerP->statusCode);
}



/* ****************************************************************************
*
* ContextElementResponse::ContextElementResponse -
*
* This constructor builds the CER object based in a BSON object taken from the
* entities collection at DB.
*
* Note that statusCode is not touched by this constructor.
*/
ContextElementResponse::ContextElementResponse
(
  const orion::BSONObj&  entityDoc,
  const StringList&      attrL,
  bool                   includeEmpty,
  ApiVersion             apiVersion
)
{
  prune = false;

  // Entity
  orion::BSONObj id = getFieldFF(entityDoc, "_id").embeddedObject();

  std::string entityId   = getStringFieldFF(id, ENT_ENTITY_ID);
  std::string entityType = id.hasField(ENT_ENTITY_TYPE) ? getStringFieldFF(id, ENT_ENTITY_TYPE) : "";

  entity.fill(entityId, entityType, "false");
  entity.servicePath = id.hasField(ENT_SERVICE_PATH) ? getStringFieldFF(id, ENT_SERVICE_PATH) : "";

  /* Get the location attribute (if it exists) */
  std::string locAttr;
  if (entityDoc.hasField(ENT_LOCATION))
  {
    locAttr = getStringFieldFF(getObjectFieldFF(entityDoc, ENT_LOCATION), ENT_LOCATION_ATTRNAME);
  }


  //
  // Attribute vector
  // FIXME P5: constructor for orion::BSONObj could be added to ContextAttributeVector/ContextAttribute classes, to make building more modular
  //
  orion::BSONObj                attrs = getObjectFieldFF(entityDoc, ENT_ATTRS);
  std::set<std::string>  attrNames;

  attrs.getFieldNames(attrNames);
  for (std::set<std::string>::iterator i = attrNames.begin(); i != attrNames.end(); ++i)
  {
    std::string        attrName                = *i;
    orion::BSONObj            attr                    = getObjectFieldFF(attrs, attrName);
    ContextAttribute*  caP                     = NULL;
    ContextAttribute   ca;
    bool               noLocationMetadata      = true;

    // Name and type
    ca.name           = dbDotDecode(attrName);
    ca.type           = getStringFieldFF(attr, ENT_ATTRS_TYPE);

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
      switch(getFieldFF(attr, ENT_ATTRS_VALUE).type())
      {
      case orion::String:
        ca.stringValue = getStringFieldFF(attr, ENT_ATTRS_VALUE);
        if (!includeEmpty && ca.stringValue.length() == 0)
        {
          continue;
        }
        caP = new ContextAttribute(ca.name, ca.type, ca.stringValue);
        break;

      case orion::NumberDouble:
        ca.numberValue = getNumberFieldFF(attr, ENT_ATTRS_VALUE);
        caP = new ContextAttribute(ca.name, ca.type, ca.numberValue);
        break;

      case orion::NumberInt:
        ca.numberValue = (double) getIntFieldFF(attr, ENT_ATTRS_VALUE);
        caP = new ContextAttribute(ca.name, ca.type, ca.numberValue);
        break;

      case orion::Bool:
        ca.boolValue = getBoolFieldFF(attr, ENT_ATTRS_VALUE);
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
        compoundObjectResponse(caP->compoundValueP, getFieldFF(attr, ENT_ATTRS_VALUE));
        break;

      case orion::Array:
        caP = new ContextAttribute(ca.name, ca.type, "");
        caP->compoundValueP = new orion::CompoundValueNode(orion::ValueTypeVector);
        caP->valueType = orion::ValueTypeVector;
        compoundVectorResponse(caP->compoundValueP, getFieldFF(attr, ENT_ATTRS_VALUE));
        break;

      default:
        LM_E(("Runtime Error (unknown attribute value type in DB: %d)", getFieldFF(attr, ENT_ATTRS_VALUE).type()));
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
      orion::BSONObj                mds = getObjectFieldFF(attr, ENT_ATTRS_MD);
      std::set<std::string>  mdsSet;

      mds.getFieldNames(mdsSet);
      for (std::set<std::string>::iterator i = mdsSet.begin(); i != mdsSet.end(); ++i)
      {
        std::string currentMd = *i;
        Metadata*   md = new Metadata(dbDotDecode(currentMd), getObjectFieldFF(mds, currentMd));

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
      caP->creDate = getNumberFieldFF(attr, ENT_ATTRS_CREATION_DATE);
    }

    if (attr.hasField(ENT_ATTRS_MODIFICATION_DATE))
    {
      caP->modDate = getNumberFieldFF(attr, ENT_ATTRS_MODIFICATION_DATE);
    }

    entity.attributeVector.push_back(caP);
  }

  /* Set creDate and modDate at entity level */
  if (entityDoc.hasField(ENT_CREATION_DATE))
  {
    entity.creDate = getNumberFieldFF(entityDoc, ENT_CREATION_DATE);
  }

  if (entityDoc.hasField(ENT_MODIFICATION_DATE))
  {
    entity.modDate = getNumberFieldFF(entityDoc, ENT_MODIFICATION_DATE);
  }
}


/* ****************************************************************************
*
* ContextElementResponse::ContextElementResponse -
*
* This constructor builds the CER from a CEP. Note that statusCode is not touched.
*/
ContextElementResponse::ContextElementResponse(Entity* eP, bool useDefaultType)
{
  entity.fill(*eP, useDefaultType);
}



/* ****************************************************************************
*
* ContextElementResponse::toJsonV1 -
*/
std::string ContextElementResponse::toJsonV1
(
  bool                             asJsonObject,
  RequestType                      requestType,
  const std::vector<std::string>&  attrsFilter,
  bool                             blacklist,
  const std::vector<std::string>&  metadataFilter,
  bool                             comma,
  bool                             omitAttributeValues
)
{
  std::string out = "";

  out += startTag();
  out += entity.toJsonV1(asJsonObject, requestType, attrsFilter, blacklist, metadataFilter, true, omitAttributeValues);
  out += statusCode.toJsonV1(false);
  out += endTag(comma, false);

  return out;
}



/* ****************************************************************************
*
* ContextElementResponse::toJson - 
*/
std::string ContextElementResponse::toJson
(
  RenderFormat                     renderFormat,
  const std::vector<std::string>&  attrsFilter,
  bool                             blacklist,
  const std::vector<std::string>&  metadataFilter
)
{
  std::string out;

  out = entity.toJson(renderFormat, attrsFilter, blacklist, metadataFilter);

  return out;
}



/* ****************************************************************************
*
* ContextElementResponse::release - 
*/
void ContextElementResponse::release(void)
{
  entity.release();
  statusCode.release();
}



/* ****************************************************************************
*
* ContextElementResponse::check - 
*/
std::string ContextElementResponse::check
(
  ApiVersion          apiVersion,
  RequestType         requestType,
  const std::string&  predetectedError,
  int                 counter
)
{
  std::string res;

  if ((res = entity.check(apiVersion, requestType)) != "OK")
  {
    return res;
  }

  if ((res = statusCode.check()) != "OK")
  {
    return res;
  }

  return "OK";
}



/* ****************************************************************************
*
* ContextElementResponse::fill - 
*/
void ContextElementResponse::fill(QueryContextResponse* qcrP, const std::string& entityId, const std::string& entityType)
{
  if (qcrP == NULL)
  {
    statusCode.fill(SccContextElementNotFound);
    return;
  }

  if (qcrP->contextElementResponseVector.size() == 0)
  {
    statusCode.fill(&qcrP->errorCode);
    entity.fill(entityId, entityType, "false");

    if ((statusCode.code != SccOk) && (statusCode.details == ""))
    {
      statusCode.details = "Entity id: /" + entityId + "/";
    }

    return;
  }

  //
  // FIXME P7: If more than one context element is found, we simply select the first one.
  //           A better approach would be to change this convop to return a vector of responses.
  //           Adding a call to alarmMgr::badInput - with this I mean that the user that sends the 
  //           query needs to avoid using this conv op to make any queries that can give more than
  //           one unique context element :-).
  //           This FIXME is related to github issue #588 and (probably) #650.
  //           Also, optimizing this would be part of issue #768
  //
  if (qcrP->contextElementResponseVector.size() > 1)
  {
    alarmMgr.badInput(clientIp, "more than one context element found the this query - selecting the first one");
  }

  entity.fill(qcrP->contextElementResponseVector[0]->entity);

  if (qcrP->errorCode.code != SccNone)
  {
    statusCode.fill(&qcrP->errorCode);
  }
}



/* ****************************************************************************
*
* ContextElementResponse::fill - 
*/
void ContextElementResponse::fill(ContextElementResponse* cerP)
{
  entity.fill(cerP->entity);
  statusCode.fill(cerP->statusCode);
}



/* ****************************************************************************
*
* ContextElementResponse::clone - 
*/
ContextElementResponse* ContextElementResponse::clone(void)
{
  ContextElementResponse* cerP = new ContextElementResponse();

  cerP->fill(this);

  return cerP;
}
