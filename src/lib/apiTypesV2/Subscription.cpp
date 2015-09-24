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

#include "Subscription.h"

#include <string>
#include <sstream>
#include <vector>

#include "common/JsonHelper.h"

using std::string;
using std::vector;

string Subscription::toJson()
{
  JsonHelper jh;

  jh.addString("id", this->id);
  if (!this->duration.isEmpty())
  {
    jh.addRaw("duration: ",this->duration.render(JSON,"", false));
  }
  jh.addRaw("subject", this->subject.toJson());
  jh.addRaw("notification", this->notification.toJson());

  return jh.str();
}

string Notification::toJson()
{
  JsonHelper jh;

  jh.addString("callback", this->callback);
  if (!this->throttling.isEmpty())
  {
    jh.addRaw("throttling", this->throttling.render(JSON,"",false));
  }
  jh.addRaw("attributes", vectorToJson(this->attributes));

  return jh.str();
}

string Subject::toJson()
{
  JsonHelper jh;

  jh.addRaw("entities", vectorToJson(this->entities));
  jh.addRaw("condition", this->condition.toJson());

  return jh.str();
}

string Condition::toJson()
{
  JsonHelper jh;

  jh.addRaw("attributes", vectorToJson(this->attributes));

  {
    JsonHelper jhe;

    jhe.addString("q", this->expression.q);
    jhe.addString("geometry", this->expression.geometry);
    jhe.addString("coords", this->expression.coords);

    jh.addRaw("expression", jhe.str());
  }

  return jh.str();
}

string EntID::toJson()
{
  JsonHelper jh;

  jh.addString("id", this->id);
  jh.addString("idPattern", this->idPattern);
  jh.addString("type", this->type);

  return jh.str();
}
