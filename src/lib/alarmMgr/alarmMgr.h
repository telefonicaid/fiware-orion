#ifndef SRC_LIB_ALARMMGR_ALARMMGR_H_
#define SRC_LIB_ALARMMGR_ALARMMGR_H_

/*
*
* Copyright 2015 Telefonica Investigacion y Desarrollo, S.A.U
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
#include "alarmMgr/AlarmManager.h"



/* ****************************************************************************
*
* clientIp - the IP address of the current peer sending the request
*
* This information is needed for all "badInput" reports for the alarm manager
* It is a thread-variable, only valid while the request is active.
* A 'global' variable was chosen instead of passing the IP from function
* to function.
*/
extern __thread char clientIp[16];  // 16: xxx.xxx.xxx.xxx\0



/* ****************************************************************************
*
* alarmMgr - 
*/
extern AlarmManager alarmMgr;

#endif  // SRC_LIB_ALARMMGR_ALARMMGR_H_
