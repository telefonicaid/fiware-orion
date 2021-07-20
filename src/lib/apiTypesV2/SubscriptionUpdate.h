#ifndef SRC_LIB_APITYPESV2_SUBSCRIPTIONUPDATE_H_
#define SRC_LIB_APITYPESV2_SUBSCRIPTIONUPDATE_H_

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
#include "apiTypesV2/Subscription.h"



namespace ngsiv2
{
/* ****************************************************************************
*
* SubscriptionUpdate - 
*/
class SubscriptionUpdate : public Subscription
{
 public:
  bool  subjectProvided;
  bool  expiresProvided;
  bool  statusProvided;
  bool  notificationProvided;
  bool  attrsFormatProvided;
  bool  maxFailsLimitProvided;
  bool  throttlingProvided;
  bool  blacklistProvided;
  bool  onlyChangedProvided;
  bool  fromNgsiv1;          // to support a special case when the SubscriptionUpdate comes from NGSIv1

  SubscriptionUpdate():
    subjectProvided(false),
    expiresProvided(false),
    statusProvided(false),
    notificationProvided(false),
    attrsFormatProvided(false),
    maxFailsLimitProvided(false),
    throttlingProvided(false),
    blacklistProvided(false),
    onlyChangedProvided(false),
    fromNgsiv1(false)
  {
    descriptionProvided = false;
  }
};
}

#endif  // SRC_LIB_APITYPESV2_SUBSCRIPTIONUPDATE_H_
