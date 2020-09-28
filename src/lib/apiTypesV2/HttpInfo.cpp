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

#include "mongo/client/dbclient.h"
#include "logMsg/logMsg.h"

#include "common/JsonHelper.h"
#include "mongoBackend/dbConstants.h"
#include "mongoBackend/safeMongo.h"
#include "apiTypesV2/HttpInfo.h"



/* ****************************************************************************
*
* USING
*/
using mongo::BSONObj;
using mongo::BSONElement;



namespace ngsiv2
{
/* ****************************************************************************
*
* HttpInfo::HttpInfo - 
*/
HttpInfo::HttpInfo() : verb(NOVERB), custom(false)
{
}



/* ****************************************************************************
*
* HttpInfo::HttpInfo - 
*/
HttpInfo::HttpInfo(const std::string& _url) : url(_url), verb(NOVERB), custom(false)
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
void HttpInfo::fill(const BSONObj& bo)
{
  this->url    = bo.hasField(CSUB_REFERENCE)? getStringFieldF(bo, CSUB_REFERENCE) : "";
  this->custom = bo.hasField(CSUB_CUSTOM)?    getBoolFieldF(bo,   CSUB_CUSTOM)    : false;

  if (this->custom)
  {
    this->payload  = bo.hasField(CSUB_PAYLOAD)? getStringFieldF(bo, CSUB_PAYLOAD) : "";
    this->includePayload = bo.hasField(CSUB_INCLUDEPAYLOAD)? getBoolFieldF(bo, CSUB_INCLUDEPAYLOAD) : false;

    if (bo.hasField(CSUB_METHOD))
    {
      this->verb = str2Verb(getStringFieldF(bo, CSUB_METHOD));
    }

    // qs
    if (bo.hasField(CSUB_QS))
    {
      BSONObj qs = getObjectFieldF(bo, CSUB_QS);

      for (BSONObj::iterator i = qs.begin(); i.more();)
      {
        BSONElement e = i.next();

        this->qs[e.fieldName()] = e.String();
      }
    }

    // headers
    if (bo.hasField(CSUB_HEADERS))
    {
      BSONObj headers = getObjectFieldF(bo, CSUB_HEADERS);

      for (BSONObj::iterator i = headers.begin(); i.more();)
      {
        BSONElement e = i.next();

        this->headers[e.fieldName()] = e.String();
      }
    }
  }
}
}
