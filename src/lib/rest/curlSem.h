#ifndef SRC_LIB_REST_CURL_H_
#define SRC_LIB_REST_CURL_H_

/*
*
* Copyright 2021 Telefonica Investigacion y Desarrollo, S.A.U
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
* curl context -
*/
struct curl_context
{
  CURL *curl;
  pthread_mutex_t *pmutex;
};



/* ****************************************************************************
*
* curl_context_cleanup - 
*/
extern void curl_context_cleanup(void);



/* ****************************************************************************
*
* get_curl_context -
*/
extern int get_curl_context(const std::string& key, struct curl_context *pcc);



/* ****************************************************************************
*
* release_curl_context -
*/
extern int release_curl_context(struct curl_context *pcc, bool final = false);



/* ****************************************************************************
*
* mutexTimeCCGet -
*/
extern float mutexTimeCCGet(void);



/* ****************************************************************************
*
* mutexTimeCCReset -
*/
void mutexTimeCCReset(void);


#endif  // SRC_LIB_REST_CURL_H_
