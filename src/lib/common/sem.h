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


// curl context includes
#include <string>

#include <pthread.h>
#include <curl/curl.h>


/* ****************************************************************************
*
* SemRequestType - 
*/
typedef enum SemRequestType
{
  SemReadOp,
  SemWriteOp,
  SemReadWriteOp,
  SemNoneOp
} SemRequestType;



/* ****************************************************************************
*
* semInit -
*/
extern int semInit
(
  SemRequestType  _reqPolicy     = SemReadWriteOp,
  bool            semTimeStat    = false,
  int             shared         = 0,
  int             takenInitially = 1
);



/* ****************************************************************************
*
* xxxSemTake -
*/
extern int reqSemTake(const char* who, const char* what, SemRequestType reqType, bool* taken);
extern int transSemTake(const char* who, const char* what);



/* ****************************************************************************
*
* xxxSemGive -
*/
extern int reqSemGive(const char* who, const char* what = NULL, bool taken = true);
extern int transSemGive(const char* who, const char* what = NULL);



/* ****************************************************************************
*
* semTimeXxxGet - get accumulated semaphore waiting time
*/
extern void semTimeReqGet(char* buf, int bufLen);
extern void semTimeTransGet(char* buf, int bufLen);



/* ****************************************************************************
*
* semTimeXxxReset - 
*/
extern void semTimeReqReset(void);
extern void semTimeTransReset(void);

/* ****************************************************************************
*
* curl context functions -
*/

struct curl_context {
  CURL *curl;
  pthread_mutex_t *pmutex;
};

/* ****************************************************************************
*
* get_curl_context -
*/
extern int get_curl_context(const std::string& key, struct curl_context *pcc);

/* ****************************************************************************
*
* release_curl_context -
*/
extern int release_curl_context(struct curl_context *pcc);

/* ****************************************************************************
*
* mutexTimeCCGet -
*/
extern void mutexTimeCCGet(char* buf, int bufLen);

/* ****************************************************************************
*
* mutexTimeCCReset -
*/
void mutexTimeCCReset(void);

#endif  // SRC_LIB_COMMON_SEM_H_
