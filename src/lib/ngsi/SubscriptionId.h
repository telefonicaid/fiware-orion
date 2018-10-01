#ifndef SRC_LIB_NGSI_SUBSCRIPTIONID_H_
#define SRC_LIB_NGSI_SUBSCRIPTIONID_H_

/*
*
* Copyright 2013 Telefonica Investigacion y Desarrollo, S.A.U
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
* Author: Ken Zangelin
*/
#include <string>

#include "ngsi/Request.h"



/* ****************************************************************************
*
* SubscriptionId - 
*/
typedef struct SubscriptionId
{
  std::string   string;

  SubscriptionId();
  SubscriptionId(const std::string& subId);

  void          set(const std::string& value);
  std::string   get(void) const;
  const char*   c_str(void) const;
  bool          isEmpty(void);
  std::string   toJsonV1(RequestType container, bool comma);
  std::string   toJson(RequestType container, bool comma);
  void          release(void);
  bool          rendered(RequestType container);

  std::string   check(void);
} SubscriptionId;

#endif  // SRC_LIB_NGSI_SUBSCRIPTIONID_H_
