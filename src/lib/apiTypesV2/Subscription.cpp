/*
*
* Copyright 2015 Telefonica Investigacion y Desarrollo, S.A.U
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
#include <sstream>
#include <vector>

#include "logMsg/logMsg.h"

#include "common/JsonHelper.h"
#include "common/globals.h"
#include "apiTypesV2/Subscription.h"



namespace ngsiv2
{
/* ****************************************************************************
*
* Subscription::~Subscription -
*/
Subscription::~Subscription()
{
  unsigned int sz = restriction.scopeVector.size();

  if (sz > 0)
  {
    for (unsigned i = 0; i != sz; i++ )
    {
      restriction.scopeVector[i]->release();
      delete restriction.scopeVector[i];
    }
    restriction.scopeVector.vec.clear();
  }
}



/* ****************************************************************************
*
* Subscription::render -
*/
std::string Subscription::render(int indent)
{
  JsonHelper writer;
  toJson(writer);
  return writer.str();
}



/* ****************************************************************************
*
* Subscription::toJson -
*/
void Subscription::toJson(JsonHelper& writer)
{
  writer.StartObject();
  writer.String("id", this->id);

  if (this->description != "")
  {
    writer.String("description", this->description);
  }

  if (this->expires != PERMANENT_SUBS_DATETIME)
  {
    writer.Date("expires", this->expires);
  }

  if ((this->notification.lastFailure > 0) && (this->notification.lastFailure > this->notification.lastSuccess))
  {
    writer.String("status", "failed");
  }
  else
  {
    writer.String("status", this->status);
  }

  writer.Key("subject");
  this->subject.toJson(writer);
  writer.Key("notification");
  this->notification.toJson(writer, renderFormatToString(this->attrsFormat, true, true));

  if (this->throttling > 0)
  {
    writer.Int("throttling", this->throttling);
  }

  writer.EndObject();
}



/* ****************************************************************************
*
* Notification::toJson -
*
* FIXME P2: we should move 'attrsFormat' from Subject class to Notification
* class, to avoid passing attrsFormat as argument
*/
void Notification::toJson(JsonHelper& writer, const std::string& attrsFormat)
{

  writer.StartObject();

  if (this->timesSent > 0)
  {
    writer.Int("timesSent", this->timesSent);
  }

  if (this->lastNotification > 0)
  {
    writer.Date("lastNotification", this->lastNotification);
  }

  if (!this->blacklist)
  {
    writer.Key("attrs");
    vectorToJson(writer, this->attributes);
  }
  else
  {
    writer.Key("exceptAttrs");
    vectorToJson(writer, this->attributes);
  }

  writer.String("attrsFormat", attrsFormat);

  if (this->httpInfo.custom)
  {
    writer.Key("httpCustom");
    this->httpInfo.toJson(writer);
  }
  else
  {
    writer.Key("http");
    this->httpInfo.toJson(writer);
  }

  if (this->metadata.size() > 0)
  {
    writer.Key("metadata");
    vectorToJson(writer, this->metadata);
  }

  if (this->lastFailure > 0)
  {
    writer.Date("lastFailure", this->lastFailure);
  }

  if (this->lastSuccess > 0)
  {
    writer.Date("lastSuccess", this->lastSuccess);
  }

  writer.EndObject();
}



/* ****************************************************************************
*
* Subject::toJson -
*/
void Subject::toJson(JsonHelper& writer)
{
  writer.Key("entities");
  vectorToJson(writer, this->entities);

  writer.Key("condition");
  this->condition.toJson(writer);
}



/* ****************************************************************************
*
* Condition::toJson -
*/
void Condition::toJson(JsonHelper& writer)
{
  writer.StartObject();

  writer.Key("attrs");
  vectorToJson(writer, this->attributes);

  if (this->expression.q != "" || this->expression.mq != "" || this->expression.geometry != "" ||
          this->expression.coords != "" || this->expression.georel != "")
  {
    writer.StartObject("expression");
    if (this->expression.q        != "")  writer.String("q",        this->expression.q);
    if (this->expression.mq       != "")  writer.String("mq",       this->expression.mq);
    if (this->expression.geometry != "")  writer.String("geometry", this->expression.geometry);
    if (this->expression.coords   != "")  writer.String("coords",   this->expression.coords);
    if (this->expression.georel   != "")  writer.String("georel",   this->expression.georel);
    writer.EndObject();
  }

  writer.EndObject();
}



/* ****************************************************************************
*
* EntID::toJson -
*/
void EntID::toJson(JsonHelper& writer)
{
  writer.StartObject();

  if (!this->id.empty())
  {
    writer.String("id", this->id);
  }

  if (!this->idPattern.empty())
  {
    writer.String("idPattern", this->idPattern);
  }

  if (!this->type.empty())
  {
    writer.String("type", this->type);
  }

  if (!this->typePattern.empty())
  {
    writer.String("typePattern", this->typePattern);
  }

  writer.EndObject();
}
}  // end namespace
