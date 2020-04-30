#ifndef SRC_LIB_APITYPESV2_SUBSCRIPTIONEXPRESSION_H_
#define SRC_LIB_APITYPESV2_SUBSCRIPTIONEXPRESSION_H_

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
* Author: Ken Zangelin
*/
#include <string>

#include "rest/StringFilter.h"


/* ****************************************************************************
*
* SubscriptionExpression -
*
* NOTE
*   This struct contains both 'q' and stringFilter.
*   q is the 'plain string' of the stringFilter and it is used in parsing of V2 Subscriptions
*   and when the q-string is read from the database.
*/
struct SubscriptionExpression
{
  SubscriptionExpression(): stringFilter(SftQ), mdStringFilter(SftMq), isSet(false) {}
  ~SubscriptionExpression() {}

  std::string               q;
  std::string               mq;
  std::string               geometry;
  std::string               coords;
  std::string               georel;
#ifdef ORIONLD
  std::string               geoproperty;
#endif

  StringFilter              stringFilter;
  StringFilter              mdStringFilter;
  bool                      isSet;
};

#endif  // SRC_LIB_APITYPESV2_SUBSCRIPTIONEXPRESSION_H_
