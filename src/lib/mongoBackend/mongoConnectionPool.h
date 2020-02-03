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
  const char* authDb,
  bool        dbSSL,
  bool        multitenant,
  double      timeout,
  int         writeConcern = 1,
  int         poolSize = 10,
  bool        semTimeStat = false
);



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



// Higher level functions (previously in MongoGlobal)

#ifdef UNIT_TEST
extern void setMongoConnectionForUnitTest(mongo::DBClientBase* _connection);
#endif


/* ****************************************************************************
*
* getMongoConnection -
*
* I would prefer to have per-collection methods, to have a better encapsulation, but
* the Mongo C++ API seems not to work that way
*/
extern mongo::DBClientBase* getMongoConnection(void);



/* ****************************************************************************
*
* releaseMongoConnection -
*/
extern void releaseMongoConnection(mongo::DBClientBase* connection);


#endif  // SRC_LIB_MONGOBACKEND_MONGOCONNECTIONPOOL_H_
