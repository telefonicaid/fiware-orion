#ifndef SRC_LIB_MONGODRIVER_MONGOCONNECTIONPOOL_H_
#define SRC_LIB_MONGODRIVER_MONGOCONNECTIONPOOL_H_

/*
*
* Copyright 2020 Telefonica Investigacion y Desarrollo, S.A.U
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
* Author: Fermín Galán
*/
// #include <semaphore.h>

#include "mongoDriver/DBConnection.h"


namespace orion
{
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
  const char* dbURI,
  const char* db,
  const char* passwd,
  bool        mtenant,
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
extern void setMongoConnectionForUnitTest(orion::DBConnection _connection);
#endif



/* ****************************************************************************
*
* getMongoConnection -
*
* I would prefer to have per-collection methods, to have a better encapsulation, but
* the Mongo C++ API seems not to work that way
*/
extern DBConnection getMongoConnection(void);



/* ****************************************************************************
*
* releaseMongoConnection -
*/
extern void releaseMongoConnection(const DBConnection& connection);
}

#endif  // SRC_LIB_MONGODRIVER_MONGOCONNECTIONPOOL_H_
