#ifndef SRC_LIB_MONGOBACKEND_MONGOCONNECTIONPOOL_H_
#define SRC_LIB_MONGOBACKEND_MONGOCONNECTIONPOOL_H_

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
#include <semaphore.h>

#include "mongo/client/dbclient.h"



/* ****************************************************************************
*
* mongoVersionGet - 
*/
extern void mongoVersionGet(int* mayor, int* minor);



/* ****************************************************************************
*
* mongoConnectionPoolInit - 
*/
extern int mongoConnectionPoolInit
(
  const char* host,
  const char* db,
  const char* rplSet,
  const char* username,
  const char* passwd,
  const char* mechanism,
  bool        multitenant,
  double      timeout,
  int         writeConcern,
  int         poolSize,
  bool        semTimeStat
);



/* ****************************************************************************
*
* mongoPoolConnectionGet - 
*/
extern mongo::DBClientBase* mongoPoolConnectionGet(void);



/* ****************************************************************************
*
* mongoPoolConnectionRelease - 
*/
extern void mongoPoolConnectionRelease(mongo::DBClientBase* connection);



/* ****************************************************************************
*
* mongoPoolConnectionSemWaitingTimeGet - 
*/
extern float mongoPoolConnectionSemWaitingTimeGet(void);



/* ****************************************************************************
*
* mongoPoolConnectionSemWaitingTimeReset - 
*/
extern void mongoPoolConnectionSemWaitingTimeReset(void);



/* ****************************************************************************
*
* mongoConnectionPoolSemGet - 
*/
extern const char* mongoConnectionPoolSemGet(void);



/* ****************************************************************************
*
* mongoConnectionSemGet - 
*/
extern const char* mongoConnectionSemGet(void);

#endif  // SRC_LIB_MONGOBACKEND_MONGOCONNECTIONPOOL_H_
