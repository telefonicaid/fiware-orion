#ifndef TRIGGERED_SUBSCRIPTION_H_
#define TRIGGERED_SUBSCRIPTION_H_

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
* fermin at tid dot es
*
* Author: Fermin Galan
*/

#include <string>

/* ****************************************************************************
*
* TriggeredSubscription -
*
* This class is thought to store the information about an ONCHANGE subscription
* triggered by an updateContext in order to notify, avoding a double-query on
* the csbubs collection. Note that adding all the BSON object retrieved from the
* csubs collection is not efficient, so we use only the needed fields-
*
*/
class TriggeredSubscripion
{
 public:
  std::string throttling;
  std::string lastNotification;
  std::string format;
  std::string reference;

  TriggeredSubscripion(const std::string&  _throttling,
                       const std::string&  _lastNotification,
                       const std::string&  _format,
                       const std::string&  _reference);

  std::string toString(const std::string& delimiter);


};

#endif // TRIGGERED_SUBSCRIPTION_H
