/*
*
* Copyright 2017 Telefonica Investigacion y Desarrollo, S.A.U
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
* Author: Fermín Galán Márquez
*/
#include <string>

#include "apiTypesV2/Registration.h"
#include "common/JsonHelper.h"
#include "common/globals.h"
#include "common/statistics.h"
#include "logMsg/logMsg.h"
#include "mongoDriver/safeMongo.h"
#include "mongoBackend/dbConstants.h"



namespace ngsiv2
{
/* ****************************************************************************
*
* ForwardingInformation::ForwardingInformation -
*/
ForwardingInformation::ForwardingInformation(): lastFailure(0), lastSuccess(0), timesSent(0), lastForwarding(0)
{
}



/* ****************************************************************************
*
* Registration::Registration -
*/
Registration::Registration(): descriptionProvided(false), expires(-1)
{
}



/* ****************************************************************************
*
* Registration::~Registration -
*/
Registration::~Registration()
{
}


/* ****************************************************************************
*
* setRegistrationId -
*/
void Registration::setRegistrationId(const orion::BSONObj& r)
{
  id = getFieldF(r, "_id").OID();
}



/* ****************************************************************************
*
* setDescription -
*/
void Registration::setDescription(const orion::BSONObj& r)
{
  if (r.hasField(REG_DESCRIPTION))
  {
    description         = getStringFieldF(r, REG_DESCRIPTION);
    descriptionProvided = true;
  }
  else
  {
    description         = "";
    descriptionProvided = false;
  }
}



/* ****************************************************************************
*
* setProvider -
*/
void Registration::setProvider(const ngsiv2::ForwardingMode forwardingMode, const std::string& format, const orion::BSONObj& r)
{
  provider.http.url = (r.hasField(REG_PROVIDING_APPLICATION))? getStringFieldF(r, REG_PROVIDING_APPLICATION): "";

  provider.supportedForwardingMode = forwardingMode;

  if (format == "JSON")
  {
    __sync_fetch_and_add(&noOfDprLegacyForwarding, 1);
    if (logDeprecate)
    {
      LM_W(("Deprecated usage of legacyForwarding mode detected in existing registration (regId: %s)", id.c_str()));
    }
    provider.legacyForwardingMode = true;
  }
  else
  {
    provider.legacyForwardingMode = false;
  }
}



/* ****************************************************************************
*
* setEntities -
*/
void Registration::setEntities(const orion::BSONObj& cr0)
{
  std::vector<orion::BSONElement>  dbEntityV = getFieldF(cr0, REG_ENTITIES).Array();

  for (unsigned int ix = 0; ix < dbEntityV.size(); ++ix)
  {
    ngsiv2::EntID    entity;
    orion::BSONObj   ce = dbEntityV[ix].embeddedObject();

    if (ce.hasField(REG_ENTITY_ISPATTERN))
    {
      std::string isPattern = getStringFieldF(ce, REG_ENTITY_ISPATTERN);

      if (isPattern == "true")
      {
        entity.idPattern = getStringFieldF(ce, REG_ENTITY_ID);
      }
      else
      {
        entity.id = getStringFieldF(ce, REG_ENTITY_ID);
      }
    }
    else
    {
      entity.id = getStringFieldF(ce, REG_ENTITY_ID);
    }

    if (ce.hasField(REG_ENTITY_ISTYPEPATTERN))
    {
      std::string isPattern = getStringFieldF(ce, REG_ENTITY_ISTYPEPATTERN);

      if (isPattern == "true")
      {
        entity.typePattern = getStringFieldF(ce, REG_ENTITY_TYPE);
      }
      else
      {
        entity.type = getStringFieldF(ce, REG_ENTITY_TYPE);
      }
    }
    else
    {
      entity.type = getStringFieldF(ce, REG_ENTITY_TYPE);
    }

    dataProvided.entities.push_back(entity);
  }
}



/* ****************************************************************************
*
* setAttributes -
*/
void Registration::setAttributes(const orion::BSONObj& cr0)
{
  std::vector<orion::BSONElement> dbAttributeV = getFieldF(cr0, REG_ATTRS).Array();

  for (unsigned int ix = 0; ix < dbAttributeV.size(); ++ix)
  {
    orion::BSONObj  aobj     = dbAttributeV[ix].embeddedObject();
    std::string     attrName = getStringFieldF(aobj, REG_ATTRS_NAME);

    if (!attrName.empty())
    {
      dataProvided.attributes.push_back(attrName);
    }
  }
}



/* ****************************************************************************
*
* setDataProvided -
*/
bool Registration::setDataProvided(const orion::BSONObj& r, bool arrayAllowed)
{
  std::vector<orion::BSONElement> crV = getFieldF(r, REG_CONTEXT_REGISTRATION).Array();

  // Only one element is allowed. This is a weird thing in the database model, see issue #4611
  if (crV.size() > 1)
  {
    return false;
  }

  // Get the forwarding mode to be used later in setProvider()
  ngsiv2::ForwardingMode forwardingMode =
      ngsiv2::stringToForwardingMode(r.hasField(REG_FORWARDING_MODE)? getStringField(r, REG_FORWARDING_MODE) : "all");

  // Get the format to be used later in setProvider()
  std::string format = r.hasField(REG_FORMAT)? getStringFieldF(r, REG_FORMAT) : "JSON";

  //
  // Extract the first (and only) CR from the contextRegistration vector
  //
  orion::BSONObj cr0 = crV[0].embeddedObject();

  setEntities(cr0);
  setAttributes(cr0);
  setProvider(forwardingMode, format, cr0);

  return true;
}



/* ****************************************************************************
*
* setExpires -
*/
void Registration::setExpires(const orion::BSONObj& r)
{
  expires = (r.hasField(REG_EXPIRATION))? getIntOrLongFieldAsLongF(r, REG_EXPIRATION) : -1;
}



/* ****************************************************************************
*
* setStatus -
*/
void Registration::setStatus(const orion::BSONObj& r)
{
  status = (r.hasField(REG_STATUS))? getStringFieldF(r, REG_STATUS): "";
}



/* ****************************************************************************
*
* Registration::fromBson -
*/
bool Registration::fromBson(const orion::BSONObj& r)
{
  setRegistrationId(r);
  setDescription(r);

  if (setDataProvided(r, false) == false)
  {
    // FIXME #4611: this check will be no longer needed after fixing the issue. setDataProvided return type (and the return type of this method)
    // could be changed to void
    return false;
  }

  setExpires(r);
  setStatus(r);

  return true;
}



/* ****************************************************************************
*
* Registration::toJson -
*/
std::string Registration::toJson(void)
{
  JsonObjectHelper jh;

  jh.addString("id", id);

  if (!description.empty())
  {
    jh.addString("description", description);
  }

  if (expires < PERMANENT_EXPIRES_DATETIME)
  {
    jh.addDate("expires", expires);
  }

  jh.addRaw("dataProvided", dataProvided.toJson());
  jh.addRaw("provider", provider.toJson());
  jh.addString("status", (!status.empty())? status : "active");

  //
  // FIXME P6: once forwarding is implemented for APIv2, include this call
  // jh.addRaw("forwardingInformation", forwardingInformation.toJson());
  //

  return jh.str();
}



/* ****************************************************************************
*
* DataProvided::toJson -
*/
std::string DataProvided::toJson(void)
{
  JsonObjectHelper jh;

  jh.addRaw("entities", vectorToJson(entities));
  jh.addRaw("attrs", vectorToJson(attributes));

  return jh.str();
}



/* ****************************************************************************
*
* Provider::toJson -
*/
std::string Provider::toJson(void)
{
  JsonObjectHelper jhUrl;
  jhUrl.addString("url", http.url);

  JsonObjectHelper   jh;
  jh.addRaw("http", jhUrl.str());
  jh.addString("supportedForwardingMode", forwardingModeToString(supportedForwardingMode));
  jh.addBool("legacyForwarding", legacyForwardingMode? true : false);

  return jh.str();
}



/* ****************************************************************************
*
* ForwardingInformation::toJson -
*/
std::string ForwardingInformation::toJson()
{
  JsonObjectHelper  jh;

  jh.addNumber("timesSent", timesSent);

  if (lastSuccess > 0)
  {
    jh.addDate("lastSuccess", lastSuccess);
  }

  if (lastFailure > 0)
  {
    jh.addDate("lastFailure", lastFailure);
  }

  if (lastForwarding > 0)
  {
    jh.addDate("lastForwarding", lastForwarding);
  }

  return jh.str();
}
}
