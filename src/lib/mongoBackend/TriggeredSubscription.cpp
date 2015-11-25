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
#include <sstream>
#include "mongoBackend/TriggeredSubscription.h"



/* ****************************************************************************
*
* TriggeredSubscription::TriggeredSubscription -
*/
TriggeredSubscription::TriggeredSubscription
(
  long long            _throttling,
  long long            _lastNotification,
  Format               _format,
  const std::string&   _reference,
  AttributeList        _attrL,
  const std::string&   _cacheSubId,
  const char*          _tenant
):
  throttling        (_throttling),
  lastNotification  (_lastNotification),
  format            (_format),
  reference         (_reference),
  attrL             (_attrL),
  cacheSubId        (_cacheSubId),
  tenant            ((_tenant == NULL)? "" : _tenant)
{

}



/* ****************************************************************************
*
* TriggeredSubscription::TriggeredSubscription -
*
* Constructor without throttling (for NGSI9 subscriptions)
*/
TriggeredSubscription::TriggeredSubscription
(
  Format             _format,
  const std::string& _reference,
  AttributeList      _attrL
):
  throttling        (-1),
  lastNotification  (-1),
  format            (_format),
  reference         (_reference),
  attrL             (_attrL),
  cacheSubId        (""),
  tenant            ("")
{

}



/* ****************************************************************************
*
* TriggeredSubscription::toString -
*/
std::string TriggeredSubscription::toString(const std::string& delimiter)
{
  std::stringstream ss;

  ss << throttling << delimiter << lastNotification << delimiter << formatToString(format) << delimiter << reference;

  return ss.str();
}
