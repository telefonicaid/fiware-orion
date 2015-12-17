#ifndef SRC_LIB_COMMON_LIMITS_H_
#define SRC_LIB_COMMON_LIMITS_H_

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



/* ****************************************************************************
*
* Service/Tenant definitions - 
*/
#define SERVICE_NAME_MAX_LEN            50
#define SERVICE_NAME_MAX_LEN_STRING    "50"


/* ****************************************************************************
*
* Service Path definitions - 
*
* The formula for SERVICE_PATH_MAX_TOTAL needs a little explanation. Considering
*
* Fiware-ServicePath: /A/B, /C/D
*
* The +1 if for the '/' in each level, the +2 is the extra characters assuming that the
* separation between tokens is ', ' (this is just an heuristic, as HTTP header could include
* more whitespace, but probably the final value is so large that it suffices most of the
* cases and don't mind in the case of truncated very long service path, from a logs point
* of view).
*
*/
#define SERVICE_PATH_MAX_COMPONENTS       10
#define SERVICE_PATH_MAX_LEVELS           10
#define SERVICE_PATH_MAX_COMPONENT_LEN    50
#define SERVICE_PATH_MAX_TOTAL            (((SERVICE_PATH_MAX_COMPONENT_LEN + 1) * SERVICE_PATH_MAX_LEVELS) + 2) * SERVICE_PATH_MAX_COMPONENTS



/* ****************************************************************************
*
* Others -
*
*/
#define IP_LENGTH_MAX        15    // Based on xxx.xxx.xxx.xxx
#define STRING_SIZE_FOR_INT  16    // Room enough for an integer



/* ****************************************************************************
*
* Alarm Manager definitions - 
*/
#define ALARM_MGR_NOTIFICATION_ERROR_LOG_SAMPLING     1
#define ALARM_MGR_BAD_INPUT_LOG_SAMPLING              1

#endif  // SRC_LIB_COMMON_LIMITS_H_
