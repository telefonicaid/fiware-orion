#ifndef SRC_LIB_APITYPESV2_SUBSCRIPTION_H_
#define SRC_LIB_APITYPESV2_SUBSCRIPTION_H_

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
#include <vector>

#include "ngsi/Duration.h"
#include "ngsi/Throttling.h"
#include "apiTypesV2/EntID.h"
#include "apiTypesV2/HttpInfo.h"
#include "apiTypesV2/SubscriptionExpression.h"
#include "ngsi/Restriction.h"
#include "common/RenderFormat.h"

namespace ngsiv2
{
/* ****************************************************************************
*
* Notification -
*/
struct Notification
{
  std::vector<std::string> attributes;
  std::vector<std::string> metadata;
  bool                     blacklist;
  long long                timesSent;
  double                   lastNotification;
  HttpInfo                 httpInfo;
  std::string              toJson(const std::string& attrsFormat);
  double                   lastFailure;
  double                   lastSuccess;
  Notification():
    attributes(),
    blacklist(false),
    timesSent(0),
    lastNotification(-1),
    httpInfo(),
    lastFailure(-1),
    lastSuccess(-1)
  {}
};



/* ****************************************************************************
*
* Condition -
*/
struct Condition
{
  std::vector<std::string>  attributes;
  SubscriptionExpression    expression;
  std::string               toJson();
};



/* ****************************************************************************
*
* Subject -
*/
struct Subject
{
  std::vector<EntID> entities;
  Condition          condition;
  std::string        toJson();
};



/* ****************************************************************************
*
* Subscription -
*/
struct Subscription
{
  std::string   id;
  std::string   description;
  bool          descriptionProvided;
  Subject       subject;
  double        expires;
  std::string   status;
  Notification  notification;
  double        throttling;
  RenderFormat  attrsFormat;
  Restriction   restriction;

#ifdef ORIONLD
  double        createdAt;
  double        modifiedAt;
  std::string   name;
  std::string   ldContext;
  int           timeInterval;
  std::string   csf;
#endif

  std::string   toJson();

  ~Subscription();
};

}  // end namespace

#endif  // SRC_LIB_APITYPESV2_SUBSCRIPTION_H_
