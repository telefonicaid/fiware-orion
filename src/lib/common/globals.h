#ifndef SRC_LIB_COMMON_GLOBALS_H_
#define SRC_LIB_COMMON_GLOBALS_H_

/*
*
* Copyright 2013 Telefonica Investigacion y Desarrollo, S.A.U
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
#include <stdint.h>

#include <string>

#include "common/Timer.h"
#include "common/sem.h"



/* ****************************************************************************
*
* FIWARE_LOCATION - 
*/
#define FIWARE_LOCATION             "FIWARE::Location"
#define FIWARE_LOCATION_DEPRECATED  "FIWARE_Location"   // Deprecated (but still supported) in Orion 0.16.0




/* ****************************************************************************
*
* useful macros
*/
#ifndef MIN
#  define MIN(a, b)     ((a) < (b)? (a) : (b))
#endif

#ifndef MAX
#  define MAX(a, b)     ((a) > (b)? (a) : (b))
#endif


/* ****************************************************************************
*
* OrionExitFunction - 
*/
typedef void (*OrionExitFunction)(int exitCode, const std::string& reason);



/* ****************************************************************************
*
* global variables
*/
extern char               fwdHost[];
extern int                fwdPort;
extern bool               ngsi9Only;
extern bool               harakiri;
extern int                startTime;
extern int                statisticsTime;
extern OrionExitFunction  orionExitFunction;
extern bool               semTimeStatistics;
extern unsigned           cprForwardLimit;
extern char               notificationMode[];
extern bool               noCache;
extern bool               simulatedNotification;



/* ****************************************************************************
*
* orionInit - 
*/
extern void orionInit(OrionExitFunction exitFunction, const char* version, SemRequestType reqPolicy, bool semTimeStat);



/* ****************************************************************************
*
* isTrue - 
*/
extern bool isTrue(const std::string& s);
extern bool isFalse(const std::string& s);



/*****************************************************************************
*
* getTimer -
*/
extern Timer* getTimer(void);



/*****************************************************************************
*
* setTimer -
*/
extern void setTimer(Timer* t);



/* ****************************************************************************
*
* getCurrentTime - 
*/ 
extern int getCurrentTime(void);



/* ****************************************************************************
*
* toSeconds -
*/
extern int64_t toSeconds(int value, char what, bool dayPart);



/*****************************************************************************
*
* parse8601 -
*
* This is common code for Duration and Throttling (at least)
*
*/
extern int64_t parse8601(const std::string& s);



/* ****************************************************************************
*
* transactionIdSet - set the transaction ID
*
* To ensure a unique identifier of the transaction, the startTime down to milliseconds
* of the broker is used as prefix (to almost guarantee its uniqueness among brokers)
* Furthermore, a running number is appended for the transaction.
* A 32 bit signed number is used, so its max value is 0x7FFFFFFF (2,147,483,647).
* If the running number overflows, a millisecond is added to the startTime.
*
* The whole thing is stored in the thread variable 'transactionId', supported by the
* logging library 'liblm'.
*
*/
extern void transactionIdSet(void);

#endif  // SRC_LIB_COMMON_GLOBALS_H_
	
