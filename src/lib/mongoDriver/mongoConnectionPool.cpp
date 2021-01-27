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
#include <time.h>
#include <semaphore.h>
#include <string>
#include <vector>

#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"

#include "common/clockFunctions.h"
#include "common/string.h"
#include "alarmMgr/alarmMgr.h"

#include "mongoDriver/connectionOperations.h"
#include "mongoDriver/safeMongo.h"
#include "mongoDriver/mongoConnectionPool.h"
#include "mongoDriver/BSONObjBuilder.h"

// FIXME OLD-DR: fix UNIT_TESTX marks in this file

/* ****************************************************************************
*
* RECONNECT_RETRIES - number of retries after connect
* RECONNECT_DELAY   - number of millisecs to sleep between retries
*/
#define RECONNECT_RETRIES 100
#define RECONNECT_DELAY   1000  // One second



/* ****************************************************************************
*
* MongoConnection -
*/
typedef struct MongoConnection
{
  orion::DBConnection  connection;
  bool                 free;
} MongoConnection;



/* ****************************************************************************
*
* globals -
*/
static MongoConnection* connectionPool     = NULL;
static int              connectionPoolSize = 0;
static sem_t            connectionPoolSem;
static sem_t            connectionSem;
static struct timespec  semWaitingTime     = { 0, 0 };
static bool             semStatistics      = false;
static int              mongoVersionMayor  = -1;
static int              mongoVersionMinor  = -1;



/* ****************************************************************************
*
* orion::mongoVersionGet -
*/
void orion::mongoVersionGet(int* mayor, int* minor)
{
  *mayor = mongoVersionMayor;
  *minor = mongoVersionMinor;
}



/* ****************************************************************************
*
* mongoConnect -
*
* Default value for writeConcern == 1 (0: unacknowledged, 1: acknowledged)
*/
static orion::DBConnection mongoConnect
(
  const char*  host,
  const char*  db,
  const char*  rplSet,
  const char*  username,
  const char*  passwd,
  const char*  mechanism,
  const char*  authDb,
  bool         multitenant,
  int          writeConcern,
  double       timeout
)
{
  std::string   err;
  mongo::DBClientBase* connection = NULL;

  // FIXME OLD-DR: remove orion:: mark
  LM_T(LmtMongo, ("orion:: Connection info: dbName='%s', rplSet='%s', timeout=%f", db, rplSet, timeout));

  bool connected     = false;
  int  retries       = RECONNECT_RETRIES;

  if (strlen(rplSet) == 0)
  {
    // No replica set. Setting the first argument to true is to use autoreconnect
    connection = new mongo::DBClientConnection(true);
  }
  else
  {
    // Replica set
    LM_T(LmtMongo, ("Using replica set %s", rplSet));

    // autoReconnect is always on for DBClientReplicaSet connections.
    std::vector<std::string>  hostTokens;
    int components = stringSplit(host, ',', hostTokens);

    std::vector<mongo::HostAndPort> rplSetHosts;
    for (int ix = 0; ix < components; ix++)
    {
      LM_T(LmtMongo, ("rplSet host <%s>", hostTokens[ix].c_str()));
      rplSetHosts.push_back(mongo::HostAndPort(hostTokens[ix]));
    }

    connection = new mongo::DBClientReplicaSet(rplSet, rplSetHosts, timeout);
  }

  for (int tryNo = 0; tryNo < retries; ++tryNo)
  {
    try
    {
      connected = (strlen(rplSet) == 0) ?
            ((mongo::DBClientConnection*) connection)->connect(std::string(host), err) :  // No repl set
            ((mongo::DBClientReplicaSet*) connection)->connect();                         // Rpls set
    }
    catch (const std::exception &e)
    {
      LM_E(("Database Startup Error (exception: %s)", ((std::string) e.what()).c_str()));
    }

    if (connected)
    {
      break;
    }

    if (tryNo == 0)
    {
      LM_E(("Database Startup Error (cannot connect to mongo - doing %d retries with a %d millisecond interval)",
            retries,
            RECONNECT_DELAY));
    }
    else
    {
      LM_T(LmtMongo, ("Try %d connecting to mongo failed", tryNo));
    }

    usleep(RECONNECT_DELAY * 1000);  // usleep accepts microseconds
  }

  if (connected == false)
  {
    char cV[64];
    snprintf(cV, sizeof(cV), "connection failed, after %d retries", retries);
    std::string details = std::string(cV) + ": " + err;

    alarmMgr.dbError(details);
    return orion::DBConnection();
  }
  alarmMgr.dbErrorReset();

  LM_T(LmtOldInfo, ("Successful connection to database"));

  // FIXME OLD-DR: no write concerns or authenticatio by the moment
#if 0
  //
  // WriteConcern
  //
  mongo::WriteConcern writeConcernCheck;

  //
  // In the legacy driver, writeConcern is no longer an int, but a class.
  // We need a small conversion step here
  //
  mongo::WriteConcern wc = writeConcern == 1 ? mongo::WriteConcern::acknowledged : mongo::WriteConcern::unacknowledged;

  setWriteConcern(connection, wc, &err);
  getWriteConcern(connection, &writeConcernCheck, &err);

  if (writeConcernCheck.nodes() != wc.nodes())
  {
    alarmMgr.dbError("Write Concern not set as desired)");
    return NULL;
  }
  alarmMgr.dbErrorReset();

  LM_T(LmtMongo, ("Active DB Write Concern mode: %d", writeConcern));

  /* Authentication is different depending if multiservice is used or not. In the case of not
   * using multiservice, we authenticate in the single-service database. In the case of using
   * multiservice, it isn't a default database that we know at contextBroker start time (when
   * this connection function is invoked) so we authenticate on the admin database, which provides
   * access to any database.
   *
   * This behaviour can be overriden by the -dbAuthDb parameter in the CLI
   */

  const char* effectiveAuthDb;
  if (strlen(authDb) > 0)
  {
    effectiveAuthDb = authDb;
  }
  else
  {
    effectiveAuthDb = multitenant ? "admin" : db;
  }

  if (strlen(db) != 0 && strlen(username) != 0 && strlen(passwd) != 0)
  {
    if (!connectionAuth(connection, std::string(effectiveAuthDb), std::string(username), std::string(passwd), std::string(mechanism), &err))
    {
      return NULL;
    }
  }
#endif

  /* Get mongo version with the 'buildinfo' command */
  orion::BSONObj  result;
  std::string     extra;

  orion::BSONObjBuilder bob;
  bob.append("buildinfo", 1);

  orion::runCollectionCommand(orion::DBConnection(connection), "admin", bob.obj(), &result, &err);
  std::string versionString = std::string(getStringFieldFF(result, "version"));
  if (!versionParse(versionString, mongoVersionMayor, mongoVersionMinor, extra))
  {
    LM_E(("Database Startup Error (invalid version format: %s)", versionString.c_str()));
    return orion::DBConnection(NULL);
  }
  LM_T(LmtMongo, ("mongo version server: %s (mayor: %d, minor: %d, extra: %s)",
                  versionString.c_str(),
                  mongoVersionMayor,
                  mongoVersionMinor,
                  extra.c_str()));

  return orion::DBConnection(connection);
}



/* ****************************************************************************
*
* shutdownClient -
*/
static void shutdownClient(void)
{
  mongo::Status status = mongo::client::shutdown();
  if (!status.isOK())
  {
    LM_E(("Database Shutdown Error %s (cannot shutdown mongo client)", status.toString().c_str()));
  }
}




/* ****************************************************************************
*
* orion::mongoConnectionPoolInit -
*/
int orion::mongoConnectionPoolInit
(
  const char*  host,
  const char*  db,
  const char*  rplSet,
  const char*  username,
  const char*  passwd,
  const char*  mechanism,
  const char*  authDb,
  bool         dbSSL,
  bool         multitenant,
  double       timeout,
  int          writeConcern,
  int          poolSize,
  bool         semTimeStat
)
{
  // We cannot move status variable declaration outside the if block. That fails at compilation time
  bool statusOk = false;
  std::string statusString;
  if (dbSSL)
  {
    mongo::Status status = mongo::client::initialize(mongo::client::Options().setSSLMode(mongo::client::Options::kSSLRequired));
    statusOk = status.isOK();
    statusString = status.toString();
  }
  else
  {
    mongo::Status status = mongo::client::initialize();
    statusOk = status.isOK();
    statusString = status.toString();
  }

  if (!statusOk)
  {
    LM_E(("Database Startup Error %s (cannot initialize mongo client)", statusString.c_str()));
    return -1;
  }
  atexit(shutdownClient);

#ifdef UNIT_TESTX
  /* Basically, we are mocking all the DB pool with a single connection. The getMongoConnection() and mongoReleaseConnection() methods
   * are mocked in similar way to ensure a coherent behaviour */
  setMongoConnectionForUnitTest(mongoConnect(host, db, rplSet, username, passwd, mechanism, authDb, multitenant, writeConcern, timeout));
  return 0;
#else
  //
  // Create the pool
  //
  connectionPool  = (MongoConnection*) calloc(sizeof(MongoConnection), poolSize);
  if (connectionPool == NULL)
  {
    LM_E(("Runtime Error (insufficient memory to create connection pool of %d connections)", poolSize));
    return -1;
  }
  connectionPoolSize = poolSize;

  //
  // Initialize (connect) the pool
  //
  for (int ix = 0; ix < connectionPoolSize; ++ix)
  {
    connectionPool[ix].free       = true;
    connectionPool[ix].connection = mongoConnect(host, db, rplSet, username, passwd, mechanism,
                                                 authDb, multitenant, writeConcern, timeout);
    if (connectionPool[ix].connection.isNull())
    {
      return -1;
    }
  }

  //
  // Set up the semaphore protecting the pool itself (connectionPoolSem)
  //
  int r = sem_init(&connectionPoolSem, 0, 1);

  if (r != 0)
  {
    LM_E(("Runtime Error (cannot create connection pool semaphore)"));
    return -1;
  }

  //
  // Set up the semaphore protecting the set of connections of the pool (connectionSem)
  // Note that this is a counting semaphore, initialized to connectionPoolSize.
  //
  r = sem_init(&connectionSem, 0, connectionPoolSize);
  if (r != 0)
  {
    LM_E(("Runtime Error (cannot create connection semaphore-set)"));
    return -1;
  }

  // Measure accumulated semaphore waiting time?
  semStatistics = semTimeStat;

  return 0;
#endif
}



/* ****************************************************************************
*
* mongoPoolConnectionGet -
*
* There are two semaphores to get a connection.
* - One binary semaphore that protects the connection-vector itself (connectionPoolSem)
* - One counting semaphore that makes the caller wait until there is at least one free connection (connectionSem)
*
* There is a limited number of connections and the first thing to do is to wait for a connection
* to become avilable (any of the N connections in the pool) - this is done waiting on the counting semaphore that is
* initialized with "POOL SIZE" - meaning the semaphore can be taken N times if the pool size is N.
*
* Once a connection is free, 'sem_wait(&connectionSem)' returns and we now have to take the semaphore
* that protects for pool itself (we *are* going to modify the vector of the pool - can only do it in
* one thread at a time ...)
*
* After taking a connection, the semaphore 'connectionPoolSem' is freed, as all modifications to the connection pool
* have finished.
* The other semaphore however, 'connectionSem', is kept and it is not freed until we finish using the connection.
*
* The function mongoPoolConnectionRelease releases the counting semaphore 'connectionSem'.
* Very important to call the function 'mongoPoolConnectionRelease' after finishing using the connection !
*
*/
static orion::DBConnection mongoPoolConnectionGet(void)
{
  orion::DBConnection connection;
  struct timespec     startTime;
  struct timespec     endTime;
  struct timespec     diffTime;

  if (semStatistics)
  {
    clock_gettime(CLOCK_REALTIME, &startTime);
  }

  sem_wait(&connectionSem);
  sem_wait(&connectionPoolSem);

  if (semStatistics)
  {
    clock_gettime(CLOCK_REALTIME, &endTime);

    clock_difftime(&endTime, &startTime, &diffTime);
    clock_addtime(&semWaitingTime, &diffTime);
  }

  for (int ix = 0; ix < connectionPoolSize; ++ix)
  {
    if (connectionPool[ix].free == true)
    {
      connectionPool[ix].free = false;
      connection = connectionPool[ix].connection;
      break;
    }
  }

  sem_post(&connectionPoolSem);

  return connection;
}


#ifndef UNIT_TESTX
/* ****************************************************************************
*
* mongoPoolConnectionRelease -
*/
static void mongoPoolConnectionRelease(const orion::DBConnection& connection)
{
  sem_wait(&connectionPoolSem);

  for (int ix = 0; ix < connectionPoolSize; ++ix)
  {
    if (connectionPool[ix].connection == connection)
    {
      connectionPool[ix].free = true;
      sem_post(&connectionSem);
      break;
    }
  }

  sem_post(&connectionPoolSem);
}
#endif



/* ****************************************************************************
*
* orion::mongoPoolConnectionSemWaitingTimeGet -
*/
float orion::mongoPoolConnectionSemWaitingTimeGet(void)
{
  return semWaitingTime.tv_sec + ((float) semWaitingTime.tv_nsec) / 1E9;
}



/* ****************************************************************************
*
* orion::mongoPoolConnectionSemWaitingTimeReset -
*/
void orion::mongoPoolConnectionSemWaitingTimeReset(void)
{
  semWaitingTime.tv_sec  = 0;
  semWaitingTime.tv_nsec = 0;
}



/* ****************************************************************************
*
* orion::mongoConnectionPoolSemGet -
*/
const char* orion::mongoConnectionPoolSemGet(void)
{
  int value;

  if (sem_getvalue(&connectionPoolSem, &value) == -1)
  {
    return "error";
  }

  if (value == 0)
  {
    return "taken";
  }

  return "free";
}



/* ****************************************************************************
*
* orion::mongoConnectionSemGet -
*/
const char* orion::mongoConnectionSemGet(void)
{
  int value;

  if (sem_getvalue(&connectionSem, &value) == -1)
  {
    return "error";
  }

  if (value == 0)
  {
    return "taken";
  }

  return "free";
}



#ifdef UNIT_TEST

static DBClientBase* connection = NULL;



/* ****************************************************************************
*
* setMongoConnectionForUnitTest -
*
* For unit tests there is only one connection. This connection is stored right here (DBClientBase* connection) and
* given out using the function getMongoConnection().
*/
void setMongoConnectionForUnitTest(DBClientBase* _connection)
{
  connection = _connection;
}



/* ****************************************************************************
*
* mongoInitialConnectionGetForUnitTest -
*
* This function is meant to be used by unit tests, to get a connection from the pool
* and then use that connection, setting it with the function 'setMongoConnectionForUnitTest'.
* This will set the static variable 'connection' in MongoGlobal.cpp and later 'getMongoConnection'
* returns that variable (getMongoConnection is used by the entire mongo backend).
*/
DBClientBase* mongoInitialConnectionGetForUnitTest(void)
{
  return mongoPoolConnectionGet();
}
#endif



/* ****************************************************************************
*
* orion::getMongoConnection -
*
* I would prefer to have per-collection methods, to have a better encapsulation, but
* the Mongo C++ API doesn't seem to work that way
*/
orion::DBConnection orion::getMongoConnection(void)
{
#ifdef UNIT_TESTX
  return connection;
#else
  return mongoPoolConnectionGet();
#endif
}



/* ****************************************************************************
*
* releaseMongoConnection - give back mongo connection to connection pool
*
* Older versions of this function planned to use a std::auto_ptr<DBClientCursor>* parameter
* in order to invoke kill() on it for a "safer" connection releasing. However, at the end
* it seems that kill() will not help at all (see deetails in https://github.com/telefonicaid/fiware-orion/issues/1568)
* and after some testing we have checked that the current solution is stable.
*/
void orion::releaseMongoConnection(const orion::DBConnection& connection)
{
#ifdef UNIT_TESTX
  return;
#else
  return mongoPoolConnectionRelease(connection);
#endif  // UNIT_TEST
}
