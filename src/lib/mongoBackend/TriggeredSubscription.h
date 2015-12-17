#ifndef SRC_LIB_MONGOBACKEND_TRIGGEREDSUBSCRIPTION_H_
#define SRC_LIB_MONGOBACKEND_TRIGGEREDSUBSCRIPTION_H_

/*
*
* Copyright 2014 Telefonica Investigacion y Desarrollo, S.A.U
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
* Author: Fermin Galan
*/

#include <string>
#include "common/Format.h"
#include "ngsi/AttributeList.h"



/* ****************************************************************************
*
* TriggeredSubscription -
*
* This class is thought to store the information about an ONCHANGE subscription
* triggered by an updateContext in order to notify, avoding a double-query on
* the csbubs collection. Note that adding all the BSON object retrieved from the
* csubs collection is not efficient, so we use only the needed fields-
*
* We use the same class for both NGSI10 and NGSI9 subscription. The only different
* is that throttling and lastNotification are not needed in the second case (note
* that there are different constructor depending the case)
*
*/
class TriggeredSubscription
{
 public:
  long long     throttling;
  long long     lastNotification;
  Format        format;
  std::string   reference;
  AttributeList attrL;
  std::string   cacheSubId;
  std::string   tenant;

  TriggeredSubscription(long long           _throttling,
                        long long           _lastNotification,
                        Format              _format,
                        const std::string&  _reference,
                        const AttributeList& _attrL,
                        const std::string&  _cacheSubId,
                        const char*         _tenant);

  TriggeredSubscription(Format               _format,
                        const std::string&   _reference,
                        const AttributeList& _attrL);

  std::string toString(const std::string& delimiter);
};

#endif  // SRC_LIB_MONGOBACKEND_TRIGGEREDSUBSCRIPTION_H_
