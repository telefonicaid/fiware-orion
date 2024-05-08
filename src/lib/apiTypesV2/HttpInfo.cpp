/*
*
* Copyright 2016 Telefonica Investigacion y Desarrollo, S.A.U
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
* Author: Orion dev team
*/
#include <string>
#include <map>

#include "logMsg/logMsg.h"
#include "common/JsonHelper.h"

#include "apiTypesV2/HttpInfo.h"

#include "mongoBackend/dbConstants.h"
#include "mongoBackend/compoundResponses.h"

#include "mongoDriver/BSONObj.h"
#include "mongoDriver/BSONElement.h"
#include "mongoDriver/safeMongo.h"


namespace ngsiv2
{
/* ****************************************************************************
*
* HttpInfo::HttpInfo - 
*/
HttpInfo::HttpInfo() : verb(NOVERB), json(NULL), payloadType(Text), custom(false), includePayload(true), timeout(0)
{
}



/* ****************************************************************************
*
* HttpInfo::toJson -
*/
std::string HttpInfo::toJson()
{
  JsonObjectHelper jh;

  jh.addString("url", this->url);

  if (this->timeout > 0)
  {
    jh.addNumber("timeout", this->timeout);
  }

  if (custom)
  {
    switch (payloadType)
    {
    case Text:
      if (!this->includePayload)
      {
        jh.addNull("payload");
      }
      else if (!this->payload.empty())
      {
        jh.addString("payload", this->payload);
      }
      break;

    case Json:
      if (this->json != NULL)
      {
        jh.addRaw("json", this->json->toJson());
      }
      break;

    case Ngsi:
      jh.addRaw("ngsi", this->ngsi.toJson(NGSI_V2_NORMALIZED, true));
      break;
    }

    if (this->verb != NOVERB)
    {
      jh.addString("method", verbName(this->verb));
    }

    if (qs.size() != 0)
    {
      jh.addRaw("qs", objectToJson(qs));
    }

    if (headers.size() != 0)
    {
      jh.addRaw("headers", objectToJson(headers));
    }
  }

  return jh.str();
}



/* ****************************************************************************
*
* HttpInfo::fill -
*/
void HttpInfo::fill(const orion::BSONObj& bo)
{
  this->url      = bo.hasField(CSUB_REFERENCE)?  getStringFieldF(bo, CSUB_REFERENCE) : "";
  this->timeout  = bo.hasField(CSUB_TIMEOUT)?    getLongFieldF(bo, CSUB_TIMEOUT)     : 0;
  this->custom   = bo.hasField(CSUB_CUSTOM)?     getBoolFieldF(bo,   CSUB_CUSTOM)    : false;

  if (this->custom)
  {
    unsigned int n = 0;
    if (bo.hasField(CSUB_PAYLOAD)) n++;
    if (bo.hasField(CSUB_JSON))    n++;
    if (bo.hasField(CSUB_NGSI))    n++;
    if (n > 1)
    {
      LM_E(("Runtime Error (custom notification must not have more than one payload related field)"));
      return;
    }

    if (bo.hasField(CSUB_PAYLOAD))
    {
      payloadType = ngsiv2::CustomPayloadType::Text;

      if (getFieldF(bo, CSUB_PAYLOAD).isNull())
      {
        // We initialize also this->payload in this case, although its value is irrelevant
        this->payload = "";
        this->includePayload = false;
      }
      else
      {
        this->payload = getStringFieldF(bo, CSUB_PAYLOAD);
        this->includePayload = true;
      }
    }
    else
    {
      this->payload = "";
      this->includePayload = true;
    }

    if (bo.hasField(CSUB_JSON))
    {
      payloadType = ngsiv2::CustomPayloadType::Json;

      orion::BSONElement be = getFieldF(bo, CSUB_JSON);
      if (be.type() == orion::Object)
      {
        // this new memory is freed in HttpInfo::release()
        this->json = new orion::CompoundValueNode(orion::ValueTypeObject);
        this->json->valueType = orion::ValueTypeObject;
        compoundObjectResponse(this->json, be);
      }
      else if (be.type() == orion::Array)
      {
        // this new memory is freed in HttpInfo::release()
        this->json = new orion::CompoundValueNode(orion::ValueTypeVector);
        this->json->valueType = orion::ValueTypeVector;
        compoundVectorResponse(this->json, be);
      }
      else
      {
        LM_E(("Runtime Error (csub json field must be Object or Array but is %s)", orion::bsonType2String(be.type())));
      }
    }
    else
    {
      this->json = NULL;
    }

    if (bo.hasField(CSUB_NGSI))
    {
      payloadType = ngsiv2::CustomPayloadType::Ngsi;

      orion::BSONObj ngsiObj = getObjectFieldF(bo, CSUB_NGSI);
      if (ngsiObj.hasField(ENT_ENTITY_ID))
      {
        this->ngsi.id = getStringFieldF(ngsiObj, ENT_ENTITY_ID);
      }
      if (ngsiObj.hasField(ENT_ENTITY_TYPE))
      {
        this->ngsi.type = getStringFieldF(ngsiObj, ENT_ENTITY_TYPE);
      }
      if (ngsiObj.hasField(ENT_ATTRS))
      {
        this->ngsi.attributeVector.fill(getObjectFieldF(ngsiObj, ENT_ATTRS));
      }
    }

    if (bo.hasField(CSUB_TIMEOUT))
    {
      this->timeout = getLongFieldF(bo, CSUB_TIMEOUT);
    }

    if (bo.hasField(CSUB_METHOD))
    {
      this->verb = str2Verb(getStringFieldF(bo, CSUB_METHOD));
    }

    // FIXME P10: toStringMap could raise exception if array elements are not strings
    // (the parsing stage should avoid the case, but better to be protetec with a catch statement)
    // qs
    if (bo.hasField(CSUB_QS))
    {
      orion::BSONObj qs = getObjectFieldF(bo, CSUB_QS);
      qs.toStringMap(&this->qs);
    }

    // FIXME P10: toStringMap could raise exception if array elements are not strings
    // (the parsing stage should avoid the case, but better to be protetec with a catch statement)
    // headers
    if (bo.hasField(CSUB_HEADERS))
    {
      orion::BSONObj headers = getObjectFieldF(bo, CSUB_HEADERS);
      headers.toStringMap(&this->headers);
    }
  }
}



/* ****************************************************************************
*
* HttpInfo::fill -
*/
void HttpInfo::fill(const HttpInfo& _httpInfo)
{
  this->url            = _httpInfo.url;
  this->verb           = _httpInfo.verb;
  this->qs             = _httpInfo.qs;
  this->headers        = _httpInfo.headers;
  this->payload        = _httpInfo.payload;
  this->payloadType    = _httpInfo.payloadType;
  this->custom         = _httpInfo.custom;
  this->includePayload = _httpInfo.includePayload;
  this->timeout        = _httpInfo.timeout;

  this->json = _httpInfo.json == NULL? NULL : _httpInfo.json->clone();

  this->ngsi.fill(_httpInfo.ngsi, false, true);  // clone compounds enabled
}



/* ****************************************************************************
*
* HttpInfo::release -
*/
void HttpInfo::release()
{
  if (json != NULL)
  {
    // This will cause the orion::CompoundValueNode destructor to be called, which
    // recursively frees all memory
    delete json;
    json = NULL;
  }
  ngsi.release();
}
}
