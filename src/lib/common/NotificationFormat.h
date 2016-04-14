#ifndef SRC_LIB_COMMON_NOTIFICATIONFORMAT_H_
#define SRC_LIB_COMMON_NOTIFICATIONFORMAT_H_

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



/* ****************************************************************************
*
* DEFAULT_NOTIFICATIONFORMAT - 
*/
#define DEFAULT_NOTIFICATION_FORMAT         NGSI_V2_NORMALIZED
#define DEFAULT_NOTIFICATION_FORMAT_STRING  "NGSIv2-NORMALIZED"



/* ****************************************************************************
*
* NotificationFormat - 
*/
typedef enum NotificationFormat
{
  NGSI_NO_NOTIFICATION_FORMAT = 0,
  NGSI_V1_JSON                = 1,
  NGSI_V2_NORMALIZED          = 2,
  NGSI_V2_KEYVALUES           = 3,
  NGSI_V2_VALUES              = 4
} NotificationFormat;



/* ****************************************************************************
*
* notificationFormatToString - 
*/
extern const char* notificationFormatToString(NotificationFormat format, bool noDefault = true);



/* ****************************************************************************
*
* stringToNotificationFormat
*/
extern NotificationFormat stringToNotificationFormat(const std::string& s, bool noDefault = false);

#endif  // SRC_LIB_COMMON_NOTIFICATIONFORMAT_H_
