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

#include "apiTypesV2/HttpInfo.h"
#include "apiTypesV2/MqttInfo.h"
#include "apiTypesV2/Expression.h"
#include "ngsi/EntityId.h"
#include "common/RenderFormat.h"

namespace ngsiv2
{
/* ****************************************************************************
*
* NotificationType -
*/
typedef enum NotificationType
{
  HttpNotification,
  MqttNotification
} NotificationType;



/* ****************************************************************************
*
* SubAltType -
*/
typedef enum SubAltType
{
  // EntityChange has been specialized into three sub-types in order to solve #4605
  // (EntityChangeBothValueAndMetadata is thre reference one used in parsing/rendering logic)
  EntityChangeBothValueAndMetadata,
  EntityChangeOnlyValue,
  EntityChangeOnlyMetadata,
  EntityUpdate,
  EntityCreate,
  EntityDelete,
  Unknown
} SubAltType;



/* ****************************************************************************
*
* Notification -
*/
struct Notification
{
  std::vector<std::string> attributes;
  std::vector<std::string> metadata;
  bool                     blacklist;
  bool                     onlyChanged;
  bool                     covered;
  long long                timesSent;
  long long                failsCounter;
  long long                maxFailsLimit;
  long long                lastNotification;
  HttpInfo                 httpInfo;     // subscription would have either httpInfo or mqttInfo, but not both
  MqttInfo                 mqttInfo;
  NotificationType         type;
  long long                lastFailure;
  long long                lastSuccess;
  std::string              lastFailureReason;
  long long                lastSuccessCode;

  std::string              toJson(const std::string& attrsFormat);
  void                     release();

  Notification():
    attributes(),
    blacklist(false),
    onlyChanged(false),
    covered(false),
    timesSent(0),
    failsCounter(0),
    maxFailsLimit(-1),
    lastNotification(-1),
    httpInfo(),
    mqttInfo(),
    type(HttpNotification),
    lastFailure(-1),
    lastSuccess(-1),
    lastFailureReason(""),
    lastSuccessCode(-1)
  {}
};



/* ****************************************************************************
*
* Condition -
*/
struct Condition
{
  std::vector<std::string>  attributes;
  Expression                expression;
  std::vector<SubAltType>   altTypes;
  bool                      notifyOnMetadataChange;
  std::string               toJson();

  Condition():
    notifyOnMetadataChange(true)
  {}
};



/* ****************************************************************************
*
* Subject -
*/
struct Subject
{
  std::vector<EntityId> entities;
  Condition             condition;
  std::string           toJson();
};



/* ****************************************************************************
*
* Subscription -
*/
class Subscription
{
public:
  std::string   id;
  std::string   description;
  bool          descriptionProvided;
  Subject       subject;
  long long     expires;
  std::string   status;
  Notification  notification;
  long long     throttling;
  RenderFormat  attrsFormat;
  std::string   toJson();
  void          release();

  Subscription():
    attrsFormat(NGSI_V2_NORMALIZED)
  {}

  ~Subscription();
};

}  // end namespace



/* ****************************************************************************
*
* parseAlterationType -
*/
extern ngsiv2::SubAltType parseAlterationType(const std::string& altType);



/* ****************************************************************************
*
* subAltType2string -
*/
extern std::string subAltType2string(ngsiv2::SubAltType altType);



/* ****************************************************************************
*
* isChangeAltType -
*/
extern bool isChangeAltType(ngsiv2::SubAltType altType);



#endif  // SRC_LIB_APITYPESV2_SUBSCRIPTION_H_
