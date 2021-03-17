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
#include <mongoc/mongoc.h>

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
* pingConnection -
*
* return:
*   0: ping is ok
*   1: ping fails due to auth
*   2: ping fails not due to auth
*/
static int pingConnection(mongoc_client_t* conn, const char* db, bool mtenat)
{
  // MongoDB has a ping command, but we are not using it, as it doesn't not
  // provides auth checking when user and pass are empty (it provides auth
  // when we have user and pass, but that is not enough).
  //
  // In addition, note that command and database depend on mtenant. If we
  // are in mtenant mode we
  // will need at some point to look for all orion* databases, so command to
  // ping will be listDatabases in the admin DB. But if we run in not mtenant
  // mode listCollections in the default database will suffice

  std::string  cmd;
  std::string  effectiveDb;

  if (mtenat)
  {
    cmd = "listDatabases";
    effectiveDb = "admin";
  }
  else
  {
    cmd = "listCollections";
    effectiveDb = db;
  }

  bson_t* ping = bson_new();
  BSON_APPEND_INT32(ping, cmd.c_str(), 1);
  mongoc_database_t* database = mongoc_client_get_database(conn, effectiveDb.c_str());

  bson_error_t error;
  int r = 0;
  if (!mongoc_database_command_with_opts(database, ping, NULL, NULL, NULL, &error))
  {
    LM_T(LmtMongo, ("ping command (%s at db %s) falied: %s", cmd.c_str(), effectiveDb.c_str(), error.message));
    // Reference error message: Authentication failed
    // Reference error message: command listCollections requires authentication
    if ((strstr(error.message, "Authentication failed") != NULL) || (strstr(error.message, "requires authentication") != NULL)) {
      r = 1;
    }
    else
    {
      r = 2;
    }
  }

  mongoc_database_destroy(database);
  bson_destroy(ping);

  return r;
}



/* ****************************************************************************
*
* mongoConnect -
*
*/
static orion::DBConnection mongoConnect
(
  const std::string  mongoUri,
  const char*        db,
  bool               mtenant,
  int                writeConcern
)
{
  // Old C++ legacy driver establishes connection at connection object
  // creation time. Note that the C driver behaves different.
  // From http://mongoc.org/libmongoc/current/connection-pooling.html#single-mode
  // "the client connects on demand when your program first uses it for a MongoDB operation".
  // Thus, we are using a ping command just to "stimulate" the connection
  //
  // FIXME #3789: does this make sense makes sense or should we just to create the
  // connection object here and let if fail (if necessary) in its first use? Note
  // current implemetation tries to use a similar approach (reconnection loop)
  // as Orion 2.6.0 did, but maybe this is completely unnecessary if we move
  // to native pool management in the C driver

  bool connected = false;
  mongoc_client_t* conn;
  for (int tryNo = 0; tryNo < RECONNECT_RETRIES; ++tryNo)
  {
    conn = mongoc_client_new(mongoUri.c_str());
    if (conn == NULL)
    {
      // If URI is wrong it is wrong for all the attemps: we early end in this case
      LM_E(("Database Startup Error (mongoUri is wrong: <%s>)", mongoUri.c_str()));
      return orion::DBConnection(NULL);
    }

    int check = pingConnection(conn, db, mtenant);

    if (check == 0)
    {
      // Connection ok
      connected = true;
      break;
    }
    else if (check == 1)
    {
      // Connection fails due to auth
      LM_E(("Database Startup Error (auth failed)"));
      mongoc_client_destroy(conn);
      return orion::DBConnection(NULL);
    }
    else  // check == 2
    {
      LM_T(LmtMongo, ("Try %d connecting to mongo failed", tryNo));
      usleep(RECONNECT_DELAY * 1000);  // usleep accepts microseconds
    }
  }

  if (!connected)
  {
    LM_E(("Database Startup Error (cannot connect to mongo - doing %d retries with a %d millisecond interval)",
          RECONNECT_RETRIES,
          RECONNECT_DELAY));
    mongoc_client_destroy(conn);
    return orion::DBConnection(NULL);
  }

  //
  // WriteConcern
  //
  // Alternatively, write concern could be established as part of the mongoUri,
  // see https://docs.mongodb.com/manual/reference/connection-string/#write-concern-options
  //
  mongoc_write_concern_t* wc = mongoc_write_concern_new();
  if (writeConcern == 0)
  {
    LM_T(LmtMongo, ("connection created with Write Concern: 0"));
    mongoc_write_concern_set_w(wc, MONGOC_WRITE_CONCERN_W_UNACKNOWLEDGED);
  }
  else   // writeConcern == 1
  {
    LM_T(LmtMongo, ("connection created with Write Concern: 1"));
    mongoc_write_concern_set_w(wc, MONGOC_WRITE_CONCERN_W_DEFAULT);
  }

  mongoc_client_set_write_concern(conn, wc);

  mongoc_write_concern_destroy(wc);

  return orion::DBConnection(conn);
}



/* ****************************************************************************
*
* shutdownClient -
*/
static void shutdownClient(void)
{
  LM_T(LmtMongo, ("shutdown mongo client"));

  for (int ix = 0; ix < connectionPoolSize; ++ix)
  {
    if (!connectionPool[ix].connection.isNull())
    {
      LM_T(LmtMongo, ("clossing connection #%d", ix));
      mongoc_client_destroy(connectionPool[ix].connection.get());
    }
  }
  mongoc_cleanup();
}



/* ****************************************************************************
*
* mongoDriverLogger -
*/
static void mongoDriverLogger
(
  mongoc_log_level_t  log_level,
  const char          *log_domain,
  const char          *message,
  void                *user_data
)
{
  // FIXME P3: don't know what to do with user_data...

  // Match driver errors to our own log levels
  switch (log_level)
  {
  case MONGOC_LOG_LEVEL_CRITICAL:
  case MONGOC_LOG_LEVEL_ERROR:
    LM_E(("mongoc driver: <%s> <%s>", log_domain, message));
    break;
  case MONGOC_LOG_LEVEL_WARNING:
  case MONGOC_LOG_LEVEL_MESSAGE:
  case MONGOC_LOG_LEVEL_INFO:
  case MONGOC_LOG_LEVEL_DEBUG:
  case MONGOC_LOG_LEVEL_TRACE:
    LM_W(("mongoc driver: <%s> <%s>", log_domain, message));
    break;
  }
}



/* ****************************************************************************
*
* mongoConnectionPoolInit -
*/
static std::string composeMongoUri
(
  const char*  host,
  const char*  rplSet,
  const char*  username,
  const char*  passwd,
  const char*  mechanism,
  const char*  authDb,
  bool         dbSSL,
  bool         dbDisableRetryWrites,
  int64_t      timeout
)
{
  // Compose the mongoUri, taking into account all information

  std::string uri = "mongodb://";

  // Add auth parameter if included
  if (strlen(username) != 0 && strlen(passwd) != 0)
  {
    uri += username + std::string(":") + passwd + "@";
  }

  uri += host + std::string("/");

  if (strlen(authDb) != 0)
  {
    uri += authDb;
  }

  // First option prefix is '?' symbol
  std::string optionPrefix = "?";

  if (strlen(rplSet) != 0)
  {
    uri += optionPrefix + "replicaSet=" + rplSet;
    optionPrefix = "&";
  }

  if (strlen(mechanism) != 0)
  {
    uri += optionPrefix + "authMechanism=" + mechanism;
    optionPrefix = "&";
  }

  if (dbSSL)
  {
    uri += optionPrefix + "tls=true&tlsAllowInvalidCertificates=true";
    optionPrefix = "&";
  }

  if (dbDisableRetryWrites)
  {
    uri += optionPrefix + "retryWrites=false";
    optionPrefix = "&";
  }

  if (timeout > 0)
  {
    char buf[STRING_SIZE_FOR_LONG];
    i2s(timeout, buf, sizeof(buf));
    uri += optionPrefix + "connectTimeoutMS=" + buf;
    optionPrefix = "&";
  }

  LM_T(LmtMongo, ("MongoDB connection URI: '%s'", offuscatePassword(uri, passwd).c_str()));

  return uri;
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
  bool         dbDisableRetryWrites,
  bool         mtenant,
  int64_t      timeout,
  int          writeConcern,
  int          poolSize,
  bool         semTimeStat
)
{
  // Contrary to intuition (as you would expect that mongoc_init is the very first
  // driver function you have to call :) the mongoc_log_set_handler function
  // has to be called *before* or some problems with some traces will occur.
  // Details at: https://jira.mongodb.org/browse/CDRIVER-3904

  // Ref: http://mongoc.org/libmongoc/current/logging.html
  mongoc_log_set_handler(mongoDriverLogger, NULL);

  mongoc_init();

  atexit(shutdownClient);

  // Set mongo Uri to connect
  std::string uri = composeMongoUri(host, rplSet, username, passwd, mechanism, authDb, dbSSL, dbDisableRetryWrites, timeout);

#ifdef UNIT_TEST
  /* Basically, we are mocking all the DB pool with a single connection. The getMongoConnection() and mongoReleaseConnection() methods
   * are mocked in similar way to ensure a coherent behaviour */
  orion::setMongoConnectionForUnitTest(mongoConnect(uri, db, mtenant, writeConcern));
  return 0;
#else
  //
  // Create the pool
  //
  connectionPool = (MongoConnection*) calloc(sizeof(MongoConnection), poolSize);
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
    connectionPool[ix].connection = mongoConnect(uri, db, mtenant, writeConcern);

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

  /* Get mongo version with the 'buildinfo' command */
  orion::BSONObj  result;
  std::string     extra;
  std::string     err;

  orion::BSONObjBuilder bob;
  bob.append("buildinfo", 1);

  if (!orion::runDatabaseCommand("admin", bob.obj(), &result, &err))
  {
    LM_X(1, ("Fatal Error (error getting MongoDB server version: %s)", err.c_str()));
  }

  std::string versionString = std::string(getStringFieldF(result, "version"));
  if (!versionParse(versionString, mongoVersionMayor, mongoVersionMinor, extra))
  {
    LM_X(1, ("Fatal Error (MongoDB invalid version format: <%s>)", versionString.c_str()));
  }
  LM_T(LmtMongo, ("mongo version server: %s (mayor: %d, minor: %d, extra: %s)",
                  versionString.c_str(),
                  mongoVersionMayor,
                  mongoVersionMinor,
                  extra.c_str()));

  LM_I(("Connected to %s (dbName: %s, poolsize: %d)", offuscatePassword(uri, passwd).c_str(), db, poolSize));

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


#ifndef UNIT_TEST
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

static orion::DBConnection connection;



/* ****************************************************************************
*
* orion::setMongoConnectionForUnitTest -
*
* For unit tests there is only one connection. This connection is stored right here (orion::DBConnection* connection) and
* given out using the function getMongoConnection().
*/
void orion::setMongoConnectionForUnitTest(orion::DBConnection _connection)
{
  connection = _connection;
}



/* ****************************************************************************
*
* mongoInitialConnectionGetForUnitTest -
*
* This function is meant to be used by unit tests, to get a connection from the pool
* and then use that connection, setting it with the function 'orion::setMongoConnectionForUnitTest'.
* This will set the static variable 'connection' in MongoGlobal.cpp and later 'getMongoConnection'
* returns that variable (getMongoConnection is used by the entire mongo backend).
*/
orion::DBConnection mongoInitialConnectionGetForUnitTest(void)
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
#ifdef UNIT_TEST
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
#ifdef UNIT_TEST
  return;
#else
  return mongoPoolConnectionRelease(connection);
#endif  // UNIT_TEST
}
