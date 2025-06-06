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
  long long                _maxFailsLimit,
  long long                _failsCounter,
  long long                _lastNotification,
  RenderFormat             _renderFormat,
  const ngsiv2::HttpInfo&  _httpInfo,
  const ngsiv2::MqttInfo&  _mqttInfo,
  const StringList&        _attrL,
  const std::string&       _cacheSubId,
  const char*              _tenant,
  bool                     _covered
)
:
  throttling(_throttling),
  maxFailsLimit(_maxFailsLimit),
  failsCounter(_failsCounter),
  lastNotification(_lastNotification),
  renderFormat(_renderFormat),
  attrL(_attrL),
  cacheSubId(_cacheSubId),
  tenant((_tenant == NULL)? "" : _tenant),
  stringFilterP(NULL),
  mdStringFilterP(NULL),
  blacklist(false),
  covered(_covered)
{
  httpInfo.fill(_httpInfo);
  mqttInfo.fill(_mqttInfo);
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

  // Only one of the release operations will actually do something
  httpInfo.release();
  mqttInfo.release();
}



/* ****************************************************************************
*
* TriggeredSubscription::fillExpression -
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
