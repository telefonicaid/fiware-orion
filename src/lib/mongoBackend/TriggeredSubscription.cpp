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
#include "apiTypesV2/HttpInfo.h"
#include "common/RenderFormat.h"
#include "mongoBackend/TriggeredSubscription.h"



/* ****************************************************************************
*
* TriggeredSubscription::TriggeredSubscription -
*/
TriggeredSubscription::TriggeredSubscription
(
  long long                _throttling,
  long long                _lastNotification,
  RenderFormat             _renderFormat,
  const ngsiv2::HttpInfo&  _httpInfo,
  const ngsiv2::MqttInfo&  _mqttInfo,
  const StringList&        _attrL,
  const std::string&       _cacheSubId,
  const char*              _tenant
)
:
  throttling(_throttling),
  lastNotification(_lastNotification),
  renderFormat(_renderFormat),
  httpInfo(_httpInfo),
  mqttInfo(_mqttInfo),
  attrL(_attrL),
  cacheSubId(_cacheSubId),
  tenant((_tenant == NULL)? "" : _tenant),
  stringFilterP(NULL),
  mdStringFilterP(NULL),
  blacklist(false)
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
  RenderFormat             _renderFormat,
  const ngsiv2::HttpInfo&  _httpInfo,
  const ngsiv2::MqttInfo&  _mqttInfo,
  const StringList&        _attrL
)
:
  throttling(-1),
  lastNotification(-1),
  renderFormat(_renderFormat),
  httpInfo(_httpInfo),
  mqttInfo(_mqttInfo),
  attrL(_attrL),
  cacheSubId(""),
  tenant(""),
  stringFilterP(NULL),
  mdStringFilterP(NULL),
  blacklist(false)
{
}



/* ****************************************************************************
*
* TriggeredSubscription::~TriggeredSubscription - 
*/
TriggeredSubscription::~TriggeredSubscription()
{
  if (stringFilterP != NULL)
  {
    delete stringFilterP;
    stringFilterP = NULL;
  }

  if (mdStringFilterP != NULL)
  {
    delete mdStringFilterP;
    mdStringFilterP = NULL;
  }
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

  ss << throttling << delimiter << lastNotification << delimiter << renderFormatToString(renderFormat) << delimiter << httpInfo.url;
  ss << expression.georel << delimiter << expression.coords << delimiter << expression.geometry << delimiter;

  return ss.str();
}



/* ****************************************************************************
*
* TriggeredSubscription::stringFilterSet - 
*/
bool TriggeredSubscription::stringFilterSet(StringFilter* _stringFilterP, std::string* errorStringP)
{
  stringFilterP = _stringFilterP->clone(errorStringP);

  return (stringFilterP == NULL)? false : true;
}



/* ****************************************************************************
*
* TriggeredSubscription::mdStringFilterSet - 
*/
bool TriggeredSubscription::mdStringFilterSet(StringFilter* _stringFilterP, std::string* errorStringP)
{
  mdStringFilterP = _stringFilterP->clone(errorStringP);

  return (mdStringFilterP == NULL)? false : true;
}
