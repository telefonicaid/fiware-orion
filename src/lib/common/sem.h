#ifndef SRC_LIB_COMMON_SEM_H_
#define SRC_LIB_COMMON_SEM_H_

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
* Author: Fermin Galan
*/
#include <stdio.h>
#include <curl/curl.h>


/* ****************************************************************************
*
* SemOpType -
*/
typedef enum SemOpType
{
  SemReadOp,
  SemWriteOp,
  SemReadWriteOp,
  SemNoneOp
} SemOpType;



/* ****************************************************************************
*
* semInit -
*/
extern int semInit
(
  SemOpType  _reqPolicy     = SemReadWriteOp,
  bool       semTimeStat    = false,
  int        shared         = 0,
  int        takenInitially = 1
);



/* ****************************************************************************
*
* reqSemTryToTake - try to take take semaphore
*
* This function is only used by the exit-function (AND only for DEBUG compilations),
* in order to clean the subscription cache.
* The exit-function runs when the broker is shutting down and instead of just wating for the
* semaphore, it is taken if it is free, if not, the subscription cache is simply cleaned.
* A little dangerous, no doubt, but, the broke is shutting down anyway, if it dies because
* of a SIGSEGV in the subscription cache code, it is not such a big deal ... perhaps ...
* This implementation will stay like this until we find a better way attack the problem.
*
* The cleanup is necessary for our memory-leak detection, to avoid finding false leaks.
*/
extern int reqSemTryToTake(void);



/* ****************************************************************************
*
* xxxSemTake -
*/
extern int reqSemTake(const char* who, const char* what, SemOpType reqType, bool* taken);
extern int transSemTake(const char* who, const char* what);
extern int cacheSemTake(const char* who, const char* what);
extern int timeStatSemTake(const char* who, const char* what);



/* ****************************************************************************
*
* xxxSemGive -
*/
extern int reqSemGive(const char* who, const char* what = NULL, bool taken = true);
extern int transSemGive(const char* who, const char* what = NULL);
extern int cacheSemGive(const char* who, const char* what = NULL);
extern int timeStatSemGive(const char* who, const char* what = NULL);



/* ****************************************************************************
*
* xxxSemGet - get the state of the semaphores
*/
extern const char* timeStatSemGet(void);
extern const char* cacheSemGet(void);
extern const char* transSemGet(void);
extern const char* reqSemGet(void);
extern const char* connectionContextSemGet(void);
extern const char* connectionSubContextSemGet(void);



/* ****************************************************************************
*
* semTimeXxxGet - get accumulated semaphore waiting time
*/
extern float semTimeReqGet(void);
extern float semTimeTransGet(void);
extern float semTimeCacheGet(void);
extern float semTimeTimeStatGet(void);



/* ****************************************************************************
*
* semTimeXxxReset - 
*/
extern void semTimeReqReset(void);
extern void semTimeTransReset(void);
extern void semTimeCacheReset(void);
extern void semTimeTimeStatReset(void);


#endif  // SRC_LIB_COMMON_SEM_H_
