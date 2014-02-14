#ifndef GLOBALS_H
#define GLOBALS_H

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
* fermin at tid dot es
*
* Author: Ken Zangelin
*/

#include <string>
#include "Timer.h"


/* ****************************************************************************
*
* PRINTF - 
*/
#define PRINTF printf


/* ****************************************************************************
*
* OrionExitFunction - 
*/
typedef void (*OrionExitFunction)(int exitCode, std::string reason);



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



/* ****************************************************************************
*
* orionInit - 
*/
extern void orionInit(OrionExitFunction exitFunction, const char* version);



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
extern Timer* getTimer();

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
extern int toSeconds(int value, char what, bool dayPart);

/*****************************************************************************
*
* parse8601 -
*
* This is common code for Duration and Throttling (at least)
*
*/
extern int parse8601(std::string s);

#endif
