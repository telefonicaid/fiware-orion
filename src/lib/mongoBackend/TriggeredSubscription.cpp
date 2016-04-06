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

#include "logMsg/logMsg.h"
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
  const AttributeList& _attrL,
  const std::string&   _cacheSubId,
  const char*          _tenant
):
  throttling              (_throttling),
  lastNotification        (_lastNotification),
  format                  (_format),
  reference               (_reference),
  attrL                   (_attrL),
  cacheSubId              (_cacheSubId),
  tenant                  ((_tenant == NULL)? "" : _tenant)
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
  Format                _format,
  const std::string&   _reference,
  const AttributeList& _attrL
):
  throttling              (-1),
  lastNotification        (-1),
  format                  (_format),
  reference               (_reference),
  attrL                   (_attrL),
  cacheSubId              (""),
  tenant                  ("")
{
}



/* ****************************************************************************
*
* TriggeredSubscription::~TriggeredSubscription - 
*/
TriggeredSubscription::~TriggeredSubscription()
{
}



/* ****************************************************************************
*
* TriggeredSubscription::fillExpression -
*
* TriggeredSubscription class is shared for NGSI9 and NGSI10 subscriptions, so it is better
* to keep expressions (an artifact for NGSI10) out of the constructor, in its independent fill
* method
*/
void TriggeredSubscription::fillExpression
(
  const std::string&  georel,
  const std::string&  geometry,
  const std::string&  coords
)
{
  expression.georel   = georel;
  expression.geometry = geometry;
  expression.coords   = coords;
}


/* ****************************************************************************
*
* TriggeredSubscription::toString -
*/
std::string TriggeredSubscription::toString(const std::string& delimiter)
{
  std::stringstream ss;

  ss << throttling << delimiter << lastNotification << delimiter << formatToString(format) << delimiter << reference;  
  ss << expression.georel << delimiter << expression.coords << delimiter << expression.geometry << delimiter;

  return ss.str();
}



/* ****************************************************************************
*
* TriggeredSubscription::stringFilterSet - 
*/
void TriggeredSubscription::stringFilterSet(StringFilter* stringFilterP)
{
  //
  // This is an object copy, like
  //   memcpy(&stringFilter, stringFilterP)
  // but including the vectors inside stringFilterP
  //
  stringFilter = *stringFilterP;
}
