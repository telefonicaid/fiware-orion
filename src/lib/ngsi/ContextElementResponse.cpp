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
#include "mongoBackend/safeMongo.h"
#include "mongoBackend/dbFieldEncoding.h"
#include "mongoBackend/compoundResponses.h"

using namespace mongo;


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
ContextElementResponse::ContextElementResponse(ContextElementResponse* cerP)
{
  prune = false;

  entity.fill(cerP->entity);
  statusCode.fill(cerP->statusCode);
}



/* ****************************************************************************
*
* includedAttribute -
*
* FIXME: note that in the current implementation, in which we only use 'name' to
* compare, this function is equal to the one for ContextRegistrationAttrribute.
* However, we keep them separated, as isDomain (present in ContextRegistrationAttribute
* but not in ContextRegistration could mean a difference). To review once domain attributes
* get implemented.
*
* FIXME #1298: domain is not going to be implemented. Unify functions
*
*/
static bool includedAttribute(const ContextAttribute& attr, const StringList& attrsV)
{
  //
  // This is the case in which the queryContextRequest doesn't include attributes,
  // so all the attributes are included in the response
  //
  if (attrsV.size() == 0 || attrsV.lookup(ALL_ATTRS))
  {
    return true;
  }

  for (unsigned int ix = 0; ix < attrsV.size(); ++ix)
  {
    if (attrsV[ix] == attr.name)
    {
      return true;
    }
  }

  return false;
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
  const mongo::BSONObj&  entityDoc,
  const StringList&      attrL,
  bool                   includeEmpty,
  ApiVersion             apiVersion
)
{
  prune = false;

  // Entity
  BSONObj id = getFieldF(entityDoc, "_id").embeddedObject();

  std::string entityId   = getStringFieldF(id, ENT_ENTITY_ID);
  std::string entityType = id.hasField(ENT_ENTITY_TYPE) ? getStringFieldF(id, ENT_ENTITY_TYPE) : "";

  entity.fill(entityId, entityType, "false");
  entity.servicePath = id.hasField(ENT_SERVICE_PATH) ? getStringFieldF(id, ENT_SERVICE_PATH) : "";

  /* Get the location attribute (if it exists) */
  std::string locAttr;
  if (entityDoc.hasElement(ENT_LOCATION))
  {
    locAttr = getStringFieldF(getObjectFieldF(entityDoc, ENT_LOCATION), ENT_LOCATION_ATTRNAME);
  }


  //
  // Attribute vector
  // FIXME P5: constructor for BSONObj could be added to ContextAttributeVector/ContextAttribute classes, to make building more modular
  //
  BSONObj                attrs = getObjectFieldF(entityDoc, ENT_ATTRS);
  std::set<std::string>  attrNames;

  attrs.getFieldNames(attrNames);
  for (std::set<std::string>::iterator i = attrNames.begin(); i != attrNames.end(); ++i)
  {
    std::string        attrName = *i;
    BSONObj            attr     = getObjectFieldF(attrs, attrName);
    ContextAttribute*  caP      = NULL;
    ContextAttribute   ca;

    // Name and type
    ca.name           = dbDotDecode(basePart(attrName));
    std::string mdId  = idPart(attrName);
    ca.type           = getStringFieldF(attr, ENT_ATTRS_TYPE);

    // Skip attribute if the attribute is in the list (or attrL is empty or includes "*")
    if (!includedAttribute(ca, attrL))
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
      case String:
        ca.stringValue = getStringFieldF(attr, ENT_ATTRS_VALUE);
        if (!includeEmpty && ca.stringValue.length() == 0)
        {
          continue;
        }
        caP = new ContextAttribute(ca.name, ca.type, ca.stringValue);
        break;

      case NumberDouble:
        ca.numberValue = getNumberFieldF(attr, ENT_ATTRS_VALUE);
        caP = new ContextAttribute(ca.name, ca.type, ca.numberValue);
        break;

      case NumberInt:
        ca.numberValue = (double) getIntFieldF(attr, ENT_ATTRS_VALUE);
        caP = new ContextAttribute(ca.name, ca.type, ca.numberValue);
        break;

      case Bool:
        ca.boolValue = getBoolFieldF(attr, ENT_ATTRS_VALUE);
        caP = new ContextAttribute(ca.name, ca.type, ca.boolValue);
        break;

      case jstNULL:
        caP = new ContextAttribute(ca.name, ca.type, "");
        caP->valueType = orion::ValueTypeNull;
        break;

      case Object:
        caP = new ContextAttribute(ca.name, ca.type, "");
        caP->compoundValueP = new orion::CompoundValueNode(orion::ValueTypeObject);
        caP->valueType = orion::ValueTypeObject;
        compoundObjectResponse(caP->compoundValueP, getFieldF(attr, ENT_ATTRS_VALUE));
        break;

      case Array:
        caP = new ContextAttribute(ca.name, ca.type, "");
        caP->compoundValueP = new orion::CompoundValueNode(orion::ValueTypeVector);
        // FIXME P7: next line is counterintuitive. If the object is a vector, why
        // we need to use ValueTypeObject here? Because otherwise Metadata::toJson()
        // method doesn't work. A littely crazy... it should be fixed.
        caP->valueType = orion::ValueTypeObject;
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

    /* Setting ID (if found) */
    if (mdId != "")
    {
      Metadata* md = new Metadata(NGSI_MD_ID, "string", mdId);
      caP->metadataVector.push_back(md);
    }

    if (apiVersion == V1)
    {
      /* Setting location metadata (if found) */
      if ((locAttr == ca.name) && (ca.type != GEO_POINT))
      {
        /* Note that if attribute type is geo:point then the user is using the "new way"
         * of locating entities in NGSIv1, thus location metadata is not rendered */
        Metadata* md = new Metadata(NGSI_MD_LOCATION, "string", LOCATION_WGS84);
        caP->metadataVector.push_back(md);
      }
    }

    /* Setting custom metadata (if any) */
    if (attr.hasField(ENT_ATTRS_MD))
    {
      BSONObj                mds = getObjectFieldF(attr, ENT_ATTRS_MD);
      std::set<std::string>  mdsSet;

      mds.getFieldNames(mdsSet);
      for (std::set<std::string>::iterator i = mdsSet.begin(); i != mdsSet.end(); ++i)
      {
        std::string currentMd = *i;
        Metadata*   md = new Metadata(dbDotDecode(currentMd), getObjectFieldF(mds, currentMd));
        caP->metadataVector.push_back(md);
      }
    }

    /* Set creDate and modDate at attribute level */
    if (attr.hasField(ENT_ATTRS_CREATION_DATE))
    {
      caP->creDate = (double) getIntOrLongFieldAsLongF(attr, ENT_ATTRS_CREATION_DATE);
    }

    if (attr.hasField(ENT_ATTRS_MODIFICATION_DATE))
    {
      caP->modDate = (double) getIntOrLongFieldAsLongF(attr, ENT_ATTRS_MODIFICATION_DATE);
    }

    entity.attributeVector.push_back(caP);
  }

  /* Set creDate and modDate at entity level */
  if (entityDoc.hasField(ENT_CREATION_DATE))
  {
    entity.creDate = (double) getIntOrLongFieldAsLongF(entityDoc, ENT_CREATION_DATE);
  }

  if (entityDoc.hasField(ENT_MODIFICATION_DATE))
  {
    entity.modDate = (double) getIntOrLongFieldAsLongF(entityDoc, ENT_MODIFICATION_DATE);
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
* ContextElementResponse::render - 
*/
std::string ContextElementResponse::render
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
  out += statusCode.render(false);
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
