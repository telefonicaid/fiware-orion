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

#include "mongoDriver/BSONObj.h"
#include "mongoDriver/BSONElement.h"
#include "mongoDriver/safeMongo.h"


namespace ngsiv2
{
/* ****************************************************************************
*
* HttpInfo::HttpInfo - 
*/
HttpInfo::HttpInfo() : verb(NOVERB), custom(false), includePayload(true)
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

  if (custom)
  {
    if (!this->includePayload)
    {
      jh.addNull("payload");
    }
    else if (!this->payload.empty())
    {
      jh.addString("payload", this->payload);
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
  this->url    = bo.hasField(CSUB_REFERENCE)? getStringFieldF(bo, CSUB_REFERENCE) : "";
  this->custom = bo.hasField(CSUB_CUSTOM)?    getBoolFieldF(bo,   CSUB_CUSTOM)    : false;

  if (this->custom)
  {
    if (bo.hasField(CSUB_PAYLOAD))
    {
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
}
