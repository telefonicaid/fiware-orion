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
#include "rest/Verb.h"                                         // verb2str

#include "orionld/mqtt/mqttParse.h"                            // mqttParse
#include "orionld/common/orionldState.h"                       // orionldState
#include "orionld/types/KeyValue.h"                            // KeyValue, keyValueAdd
#include "orionld/types/Verb.h"                                // Verb

#include "apiTypesV2/HttpInfo.h"                               // Own interface



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
#ifdef ORIONLD
HttpInfo::HttpInfo() : verb(HTTP_NOVERB), custom(false), mimeType(MT_DEFAULT)
#else
HttpInfo::HttpInfo() : verb(HTTP_NOVERB), custom(false)
#endif
{
#ifdef ORIONLD
  bzero(&mqtt, sizeof(mqtt));
#endif
}



/* ****************************************************************************
*
* HttpInfo::HttpInfo -
*/
#ifdef ORIONLD
HttpInfo::HttpInfo(const std::string& _url) : url(_url), verb(HTTP_NOVERB), custom(false), mimeType(MT_DEFAULT)
#else
HttpInfo::HttpInfo(const std::string& _url) : url(_url), verb(HTTP_NOVERB), custom(false)
#endif
{
#ifdef ORIONLD
  bzero(&mqtt, sizeof(mqtt));
#endif
}



/* ****************************************************************************
*
* HttpInfo::toJson -
*/
std::string HttpInfo::toJson()
{
  JsonHelper jh;

  jh.addString("url", this->url);

  if (custom)
  {
    if (this->payload != "")
    {
      jh.addString("payload", this->payload);
    }

    if (this->verb != HTTP_NOVERB)
    {
      jh.addString("method", verbToString(this->verb));
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
void HttpInfo::fill(const BSONObj* boP)
{
  this->url    = boP->hasField(CSUB_REFERENCE)? getStringFieldF(boP,  CSUB_REFERENCE) : "";
  this->custom = boP->hasField(CSUB_CUSTOM)?    getBoolFieldF(boP,    CSUB_CUSTOM)    : false;

  bool  mqtt           = (strncmp(url.c_str(), "mqtt", 4) == 0);
  char* mimeTypeString = (char*) getStringFieldF(boP, CSUB_MIMETYPE);

  if (mimeTypeString[0] == 0)
    mimeTypeString = (char*) "application/json";  // Default value

  this->mimeType = longStringToMimeType(mimeTypeString);

  if (boP->hasField("headers"))  // This is 'receiverInfo' from the NGSI-LD API
  {
    mongo::BSONObj headersObj;

    getObjectFieldF(&headersObj, boP, "headers");  // If it fails, 'headersObj' is empty and the for-loop will not be entered

    for (BSONObj::iterator iter = headersObj.begin(); iter.more();)
    {
      mongo::BSONElement be  = iter.next();
      const char*        key = be.fieldName();

      headers[key] = be.String();
    }
  }

  if (boP->hasField("notifierInfo"))
  {
    mongo::BSONObj ni;

    getObjectFieldF(&ni, boP, "notifierInfo");  // If it fails, 'ni' is empty and the for-loop will not be entered

    for (BSONObj::iterator iter = ni.begin(); iter.more();)
    {
      mongo::BSONElement be = iter.next();

      const char*  key   = be.fieldName();
      const char*  value = be.String().c_str();

      keyValueAdd(&notifierInfo, key, value);
    }
  }

  if (this->custom)
  {
    this->payload  = boP->hasField(CSUB_PAYLOAD)? getStringFieldF(boP, CSUB_PAYLOAD) : "";

    if (boP->hasField(CSUB_METHOD))
    {
      this->verb = str2Verb(getStringFieldF(boP, CSUB_METHOD));
    }

    // qs
    if (boP->hasField(CSUB_QS))
    {
      mongo::BSONObj qs;

      getObjectFieldF(&qs, boP, CSUB_QS);  // If it fails, 'qs' is empty and the for-loop will not be entered

      for (BSONObj::iterator i = qs.begin(); i.more();)
      {
        BSONElement e = i.next();

        this->qs[e.fieldName()] = e.String();
      }
    }
  }

  if (this->custom || mqtt)
  {
    // headers
    if (boP->hasField(CSUB_HEADERS))
    {
      BSONObj headers;

      getObjectFieldF(&headers, boP, CSUB_HEADERS);  // If it fails, 'headers' is empty and the for-loop will not be entered

      for (BSONObj::iterator i = headers.begin(); i.more();)
      {
        BSONElement e = i.next();

        this->headers[e.fieldName()] = e.String();
      }
    }
  }

  if (mqtt)
  {
    char*           url           = strdup(this->url.c_str());
    bool            mqtts         = false;
    char*           mqttUser      = NULL;
    char*           mqttPassword  = NULL;
    char*           mqttHost      = NULL;
    unsigned short  mqttPort      = 0;
    char*           mqttTopic     = NULL;
    char*           detail        = NULL;

    if (mqttParse(url, &mqtts, &mqttUser, &mqttPassword, &mqttHost, &mqttPort, &mqttTopic, &detail) == false)
    {
      free(url);
      LM_E(("Internal Error (unable to parse mqtt URL)"));
      return;
    }

    if (mqttUser     != NULL) strncpy(this->mqtt.username, mqttUser,     sizeof(this->mqtt.username) - 1);
    if (mqttPassword != NULL) strncpy(this->mqtt.password, mqttPassword, sizeof(this->mqtt.password) - 1);
    if (mqttHost     != NULL) strncpy(this->mqtt.host,     mqttHost,     sizeof(this->mqtt.host) - 1);
    if (mqttTopic    != NULL) strncpy(this->mqtt.topic,    mqttTopic,    sizeof(this->mqtt.topic) - 1);

    this->mqtt.mqtts = mqtts;
    this->mqtt.port  = mqttPort;

    free(url);
  }
}

}  // namespace ngsiv2
