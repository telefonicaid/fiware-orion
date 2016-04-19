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
#include <string.h>
#include <string>
#include <sstream>

#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"
#include "common/NotificationFormat.h"
#include "common/wsStrip.h"



/* ****************************************************************************
*
* notificationFormatToString - 
*/
const char* notificationFormatToString(NotificationFormat format, bool noDefault)
{
  switch (format)
  {
  case NGSI_V1_JSON:                return "JSON";
  case NGSI_V2_NORMALIZED:          return "normalized";
  case NGSI_V2_KEYVALUES:           return "keyvalues";
  case NGSI_V2_VALUES:              return "values";
  case NGSI_NO_NOTIFICATION_FORMAT:
    if (noDefault == true)
    {
      return "no notification format";
    }
    else
    {
      return "JSON";
    }
  }

  return "Unknown notification format";
}



/* ****************************************************************************
*
* stringToNotificationFormat -
*/
NotificationFormat stringToNotificationFormat(const std::string& s, bool noDefault)
{
  if (s == "JSON")       { return NGSI_V1_JSON;       }
  if (s == "normalized") { return NGSI_V2_NORMALIZED; }
  if (s == "keyvalues")  { return NGSI_V2_KEYVALUES;  }
  if (s == "values")     { return NGSI_V2_VALUES;     }
  
  return (noDefault == false)? DEFAULT_NOTIFICATION_FORMAT : NGSI_NO_NOTIFICATION_FORMAT;
}
