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
* Subscription::toJson -
*/
std::string Subscription::toJson(void)
{
  JsonObjectHelper jh;

  jh.addString("id", this->id);

  if (!this->description.empty())
  {
    jh.addString("description", this->description);
  }

  // Orion versions previous to 1.13.0 where using (int64_t) 9e18 as expiration for permanent
  // subscriptions. Now we use PERMANENT_EXPIRES_DATETIME, whichs is larger, but we need to be prepared
  // for these old documents in DB (more info in issue #3256)
  if (this->expires < (int64_t) 9e18)
  {
    jh.addDate("expires", this->expires);
  }

  if ((this->notification.lastFailure > 0) && (this->notification.lastFailure > this->notification.lastSuccess))
  {
    jh.addString("status", "failed");
  }

  else
  {
    jh.addString("status", this->status);
  }

  jh.addRaw("subject", this->subject.toJson());
  jh.addRaw("notification", this->notification.toJson(renderFormatToString(this->attrsFormat, true, true)));

  if (this->throttling > 0)
  {
    jh.addNumber("throttling", this->throttling);
  }

  if (this->notification.counter > this->notification.maxLimit)
  {
    jh.addString("status", "inactive");
  }


  return jh.str();
}



/* ****************************************************************************
*
* Notification::toJson -
*
* FIXME P2: we should move 'attrsFormat' from Subject class to Notification
* class, to avoid passing attrsFormat as argument
*/
std::string Notification::toJson(const std::string& attrsFormat)
{
  JsonObjectHelper jh;

  if (this->timesSent > 0)
  {
    jh.addNumber("timesSent", this->timesSent);
  }

  if (this->counter > 0)
  {
    jh.addNumber("counter", this->counter);
  }

  if (this->maxLimit > 0)
  {
    jh.addNumber("maxLimit", this->maxLimit);
  }

  if (this->lastNotification > 0)
  {
    jh.addDate("lastNotification", this->lastNotification);
  }

  if (!this->blacklist && !this->onlyChanged)
  {
    jh.addRaw("attrs", vectorToJson(this->attributes));
    jh.addBool("onlyChangedAttrs", false);
  }
  else if (!this->blacklist && this->onlyChanged)
  {
    jh.addRaw("attrs", vectorToJson(this->attributes));
    jh.addBool("onlyChangedAttrs", this->onlyChanged);
  }
  else if (this->blacklist && this->onlyChanged)
  {
    jh.addRaw("exceptAttrs", vectorToJson(this->attributes));
    jh.addBool("onlyChangedAttrs", this->onlyChanged);
  }
  else
  {
    jh.addRaw("exceptAttrs", vectorToJson(this->attributes));
    jh.addBool("onlyChangedAttrs", false);
  }

  jh.addString("attrsFormat", attrsFormat);

  if (this->httpInfo.custom)
  {
    jh.addRaw("httpCustom", this->httpInfo.toJson());
  }
  else
  {
    jh.addRaw("http", this->httpInfo.toJson());
  }

  if (this->metadata.size() > 0)
  {
    jh.addRaw("metadata", vectorToJson(this->metadata));
  }

  if (this->lastFailure > 0)
  {
    jh.addDate("lastFailure", this->lastFailure);
  }

  if (!this->lastFailureReason.empty())
  {
    jh.addString("lastFailureReason", this->lastFailureReason);
  }

  if (this->lastSuccess > 0)
  {
    jh.addDate("lastSuccess", this->lastSuccess);
  }

  if (this->lastSuccessCode != -1)
  {
    jh.addNumber("lastSuccessCode", this->lastSuccessCode);
  }

  return jh.str();
}



/* ****************************************************************************
*
* Subject::toJson -
*/
std::string Subject::toJson()
{
  JsonObjectHelper jh;

  jh.addRaw("entities", vectorToJson(this->entities));
  jh.addRaw("condition", this->condition.toJson());

  return jh.str();
}



/* ****************************************************************************
*
* Condition::toJson -
*/
std::string Condition::toJson()
{
  JsonObjectHelper jh;

  jh.addRaw("attrs", vectorToJson(this->attributes));

  JsonObjectHelper jhe;

  if (!this->expression.q.empty())        jhe.addString("q",        this->expression.q);
  if (!this->expression.mq.empty())       jhe.addString("mq",       this->expression.mq);
  if (!this->expression.geometry.empty()) jhe.addString("geometry", this->expression.geometry);
  if (!this->expression.coords.empty())   jhe.addString("coords",   this->expression.coords);
  if (!this->expression.georel.empty())   jhe.addString("georel",   this->expression.georel);

  std::string expressionString = jhe.str();

  if (expressionString != "{}")         jh.addRaw("expression", expressionString);

  return jh.str();
}


}  // end namespace
