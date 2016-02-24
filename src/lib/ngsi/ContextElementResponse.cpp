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

#include "common/Format.h"
#include "common/tag.h"
#include "alarmMgr/alarmMgr.h"
#include "ngsi/ContextElementResponse.h"
#include "ngsi/AttributeList.h"
#include "ngsi10/QueryContextResponse.h"
#include "rest/ConnectionInfo.h"

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

  contextElement.entityId.fill(eP);

  if (aP != NULL)
  {
    contextElement.contextAttributeVector.push_back(new ContextAttribute(aP));
  }
}



/* ****************************************************************************
*
* ContextElementResponse::ContextElementResponse - 
*/
ContextElementResponse::ContextElementResponse(ContextElementResponse* cerP)
{
  prune = false;

  contextElement.fill(cerP->contextElement);
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
*/
static bool includedAttribute(const ContextAttribute& attr, const AttributeList& attrsV)
{
  //
  // This is the case in which the queryContextRequest doesn't include attributes,
  // so all the attributes are included in the response
  //
  if (attrsV.size() == 0)
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
  const AttributeList&   attrL,
  bool                   includeEmpty,
  bool                   includeCreDate,
  bool                   includeModDate,
  const std::string&     apiVersion
)
{
  prune = false;

  // Entity
  BSONObj id = getField(entityDoc, "_id").embeddedObject();
  contextElement.entityId.fill(getStringField(id, ENT_ENTITY_ID), getStringField(id, ENT_ENTITY_TYPE), "false");
  contextElement.entityId.servicePath = id.hasField(ENT_SERVICE_PATH) ? getStringField(id, ENT_SERVICE_PATH) : "";

  /* Get the location attribute (if it exists) */
  std::string locAttr;
  if (entityDoc.hasElement(ENT_LOCATION))
  {
    locAttr = getStringField(getObjectField(entityDoc, ENT_LOCATION), ENT_LOCATION_ATTRNAME);
  }


  //
  // Attribute vector
  // FIXME P5: constructor for BSONObj could be added to ContextAttributeVector/ContextAttribute classes, to make building more modular
  //
  BSONObj                attrs = getField(entityDoc, ENT_ATTRS).embeddedObject();
  std::set<std::string>  attrNames;

  attrs.getFieldNames(attrNames);
  for (std::set<std::string>::iterator i = attrNames.begin(); i != attrNames.end(); ++i)
  {
    std::string        attrName = *i;
    BSONObj            attr     = getField(attrs, attrName).embeddedObject();
    ContextAttribute*  caP      = NULL;
    ContextAttribute   ca;

    // Name and type
    ca.name           = dbDotDecode(basePart(attrName));
    std::string mdId  = idPart(attrName);
    ca.type           = getStringField(attr, ENT_ATTRS_TYPE);

    // Skip attribute if the attribute is in the list (or attrL is empty)
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
      switch(getField(attr, ENT_ATTRS_VALUE).type())
      {
      case String:
        ca.stringValue = getStringField(attr, ENT_ATTRS_VALUE);
        if (!includeEmpty && ca.stringValue.length() == 0)
        {
          continue;
        }
        caP = new ContextAttribute(ca.name, ca.type, ca.stringValue);
        break;

      case NumberDouble:
        ca.numberValue = getField(attr, ENT_ATTRS_VALUE).Number();
        caP = new ContextAttribute(ca.name, ca.type, ca.numberValue);
        break;

      case NumberInt:
        ca.numberValue = (double) getIntField(attr, ENT_ATTRS_VALUE);
        caP = new ContextAttribute(ca.name, ca.type, ca.numberValue);
        break;

      case Bool:
        ca.boolValue = getBoolField(attr, ENT_ATTRS_VALUE);
        caP = new ContextAttribute(ca.name, ca.type, ca.boolValue);
        break;

      case jstNULL:
        caP = new ContextAttribute(ca.name, ca.type, "");
        caP->valueType = orion::ValueTypeNone;
        break;

      case Object:
        caP = new ContextAttribute(ca.name, ca.type, "");
        caP->compoundValueP = new orion::CompoundValueNode(orion::ValueTypeObject);
        compoundObjectResponse(caP->compoundValueP, getField(attr, ENT_ATTRS_VALUE));
        break;

      case Array:
        caP = new ContextAttribute(ca.name, ca.type, "");
        caP->compoundValueP = new orion::CompoundValueNode(orion::ValueTypeVector);
        compoundVectorResponse(caP->compoundValueP, getField(attr, ENT_ATTRS_VALUE));
        break;

      default:
        LM_E(("Runtime Error (unknown attribute value type in DB: %d)", getField(attr, ENT_ATTRS_VALUE).type()));
      }
    }

    /* Setting ID (if found) */
    if (mdId != "")
    {
      Metadata* md = new Metadata(NGSI_MD_ID, "string", mdId);
      caP->metadataVector.push_back(md);
    }

    if (apiVersion == "v1")
    {
      /* Setting location metadata (if found) */
      if (locAttr == ca.name)
      {
        Metadata* md = new Metadata(NGSI_MD_LOCATION, "string", LOCATION_WGS84);
        caP->metadataVector.push_back(md);
      }
    }

    /* Setting custom metadata (if any) */
    if (attr.hasField(ENT_ATTRS_MD))
    {
      std::vector<BSONElement> metadataV = getField(attr, ENT_ATTRS_MD).Array();

      for (unsigned int ix = 0; ix < metadataV.size(); ++ix)
      {
        Metadata* md = new Metadata(metadataV[ix].embeddedObject());
        caP->metadataVector.push_back(md);
      }
    }

    contextElement.contextAttributeVector.push_back(caP);
  }

  /* creDate and modDate as "virtual" attributes. The entityDoc.hasField(...) part is a safety meassure to prevent entities created with
   * very old Orion version which didn't implement creation/modification date */
  if (includeCreDate && entityDoc.hasField(ENT_CREATION_DATE))
  {
    ContextAttribute* caP = new ContextAttribute(DATE_CREATED, DATE_TYPE, (double) getIntOrLongFieldAsLong(entityDoc, ENT_CREATION_DATE));
    contextElement.contextAttributeVector.push_back(caP);
  }

  if (includeModDate && entityDoc.hasField(ENT_MODIFICATION_DATE))
  {
    ContextAttribute* caP = new ContextAttribute(DATE_MODIFIED, DATE_TYPE, (double) getIntOrLongFieldAsLong(entityDoc, ENT_MODIFICATION_DATE));
    contextElement.contextAttributeVector.push_back(caP);
  }
}



/* ****************************************************************************
*
* ContextElementResponse::ContextElementResponse -
*
* This constructor builds the CER from a CEP. Note that statusCode is not touched.
*/
ContextElementResponse::ContextElementResponse(ContextElement* ceP, bool useDefaultType)
{
  contextElement.fill(ceP, useDefaultType);
}



/* ****************************************************************************
*
* ContextElementResponse::render - 
*/
std::string ContextElementResponse::render
(
  ConnectionInfo*     ciP,
  RequestType         requestType,
  const std::string&  indent,
  bool                comma,
  bool                omitAttributeValues
)
{
  std::string xmlTag   = "contextElementResponse";
  std::string jsonTag  = "contextElement";
  std::string out      = "";

  out += startTag(indent, xmlTag, jsonTag, ciP->outFormat, false, false);
  out += contextElement.render(ciP, requestType, indent + "  ", true, omitAttributeValues);
  out += statusCode.render(ciP->outFormat, indent + "  ", false);
  out += endTag(indent, xmlTag, ciP->outFormat, comma, false);

  return out;
}



/* ****************************************************************************
*
* ContextElementResponse::release - 
*/
void ContextElementResponse::release(void)
{
  contextElement.release();
  statusCode.release();
}



/* ****************************************************************************
*
* ContextElementResponse::check - 
*/
std::string ContextElementResponse::check
(
  ConnectionInfo*     ciP,
  RequestType         requestType,
  Format              format,
  const std::string&  indent,
  const std::string&  predetectedError,
  int                 counter
)
{
  std::string res;

  if ((res = contextElement.check(ciP, requestType, format, indent, predetectedError, counter)) != "OK")
  {
    return res;
  }

  if ((res = statusCode.check(requestType, format, indent, predetectedError, counter)) != "OK")
  {
    return res;
  }

  return "OK";
}



/* ****************************************************************************
*
* ContextElementResponse::present - 
*/
void ContextElementResponse::present(const std::string& indent, int ix)
{
  LM_T(LmtPresent, ("%sContextElementResponse %d:", 
		    indent.c_str(), 
		    ix));
  contextElement.present(indent + "  ", ix);
  statusCode.present(indent + "  ");
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
    contextElement.entityId.fill(entityId, entityType, "false");

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

  contextElement.fill(&qcrP->contextElementResponseVector[0]->contextElement);

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
  contextElement.fill(cerP->contextElement);
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
