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
#include <string>

#include "mongo/client/dbclient.h"
// #include "mongo/client/index_spec.h"  // FIXME OLD-DR: to be needed soon

#include "logMsg/traceLevels.h"
#include "logMsg/logMsg.h"
#include "common/string.h"
#include "common/statistics.h"
#include "common/clockFunctions.h"
#include "alarmMgr/alarmMgr.h"

#include "mongoDriver/connectionOperations.h"
#include "mongoDriver/mongoConnectionPool.h"



/* ****************************************************************************
*
* orion::collectionQuery -
*
* Different from others, this function doesn't use getMongoConnection() and
* releaseMongoConnection(). It is assumed that the caller will do, as the
* connection cannot be released before the cursor has been used.
*/
bool orion::collectionQuery
(
  orion::DBConnection    _connection,
  const std::string&     col,
  const orion::BSONObj&  _q,
  orion::DBCursor*       cursor,
  std::string*           err
)
{
  // Getting the "low level" driver objects
  mongo::DBClientBase* connection = _connection.get();
  const mongo::BSONObj q          = _q.get();

  if (connection == NULL)
  {
    LM_E(("Fatal Error (null DB connection)"));
    *err = "null DB connection";

    return false;
  }

  LM_T(LmtMongo, ("query() in '%s' collection: '%s'", col.c_str(), q.toString().c_str()));

  try
  {
    // Setting the call cursor with "low level" cursor
    cursor->set(connection->query(col.c_str(), q));

    // We have observed that in some cases of DB errors (e.g. the database daemon is down) instead of
    // raising an exception, the query() method sets the cursor to NULL. In this case, we raise the
    // exception ourselves
    //
    if (cursor->isNull())
    {
      throw mongo::DBException("Null cursor from mongo (details on this is found in the source code)", 0);
    }
    LM_I(("Database Operation Successful (query: %s)", q.toString().c_str()));
  }
  catch (const std::exception &e)
  {
    std::string msg = std::string("collection: ") + col +
      " - query(): " + q.toString() +
      " - exception: " + e.what();

    *err = "Database Error (" + msg + ")";
    alarmMgr.dbError(msg);

    return false;
  }
  catch (...)
  {
    std::string msg = std::string("collection: ") + col +
      " - query(): " + q.toString() +
      " - exception: generic";

    *err = "Database Error (" + msg + ")";
    alarmMgr.dbError(msg);

    return false;
  }

  alarmMgr.dbErrorReset();
  return true;
}



/* ****************************************************************************
*
* orion::collectionRangedQuery -
*
* Different from others, this function doesn't use getMongoConnection() and
* releaseMongoConnection(). It is assumed that the caller will do, as the
* connection cannot be released before the cursor has been used.
*/
bool orion::collectionRangedQuery
(
  orion::DBConnection    _connection,
  const std::string&     col,
  const orion::BSONObj&  _q,
  const orion::BSONObj&  _sort,  // FIXME: this change can be propagated independtly to master
  int                    limit,
  int                    offset,
  orion::DBCursor*       cursor,
  long long*             count,
  std::string*           err
)
{
  // Getting the "low level" driver objects
  mongo::DBClientBase* connection = _connection.get();

  // Compose the query
  mongo::Query query(_q.get());
  query.sort(_sort.get());

  if (connection == NULL)
  {
    LM_E(("Fatal Error (null DB connection)"));
    *err = "null DB connection";

    return false;
  }

  LM_T(LmtMongo, ("query() in '%s' collection limit=%d, offset=%d: '%s'",
                  col.c_str(),
                  limit,
                  offset,
                  query.toString().c_str()));

  try
  {
    if (count != NULL)
    {
      *count = connection->count(col.c_str(), _q.get());
    }

    // Setting the call cursor with "low level" cursor
    cursor->set(connection->query(col.c_str(), query, limit, offset));

    //
    // We have observed that in some cases of DB errors (e.g. the database daemon is down) instead of
    // raising an exception, the query() method sets the cursor to NULL. In this case, we raise the
    // exception ourselves
    //
    if (cursor->isNull())
    {
      throw mongo::DBException("Null cursor from mongo (details on this is found in the source code)", 0);
    }
    LM_I(("Database Operation Successful (query: %s)", query.toString().c_str()));
  }
  catch (const std::exception &e)
  {
    std::string msg = std::string("collection: ") + col.c_str() +
      " - query(): " + query.toString() +
      " - exception: " + e.what();

    *err = "Database Error (" + msg + ")";
    alarmMgr.dbError(msg);

    return false;
  }
  catch (...)
  {
    std::string msg = std::string("collection: ") + col.c_str() +
      " - query(): " + query.toString() +
      " - exception: generic";

    *err = "Database Error (" + msg + ")";
    alarmMgr.dbError(msg);

    return false;
  }

  alarmMgr.dbErrorReset();
  return true;
}


/* ****************************************************************************
*
* orion::collectionCount -
*/
bool orion::collectionCount
(
  const std::string&     col,
  const orion::BSONObj&  _q,
  unsigned long long*    c,
  std::string*           err
)
{
  TIME_STAT_MONGO_READ_WAIT_START();

  orion::DBConnection connection = orion::getMongoConnection();

  // Getting the "low level" driver objects
  const mongo::BSONObj q = _q.get();

  if (connection.isNull())
  {
    TIME_STAT_MONGO_READ_WAIT_STOP();
    LM_E(("Fatal Error (null DB connection)"));
    *err = "null DB connection";

    return false;
  }

  LM_T(LmtMongo, ("count() in '%s' collection: '%s'", col.c_str(), q.toString().c_str()));

  try
  {
    *c = connection.get()->count(col.c_str(), q);
    orion::releaseMongoConnection(connection);
    TIME_STAT_MONGO_READ_WAIT_STOP();
    LM_I(("Database Operation Successful (count: %s)", q.toString().c_str()));
  }
  catch (const std::exception& e)
  {
    orion::releaseMongoConnection(connection);
    TIME_STAT_MONGO_READ_WAIT_STOP();

    std::string msg = std::string("collection: ") + col.c_str() +
      " - count(): " + q.toString() +
      " - exception: " + e.what();

    *err = "Database Error (" + msg + ")";
    alarmMgr.dbError(msg);

    return false;
  }
  catch (...)
  {
    orion::releaseMongoConnection(connection);
    TIME_STAT_MONGO_READ_WAIT_STOP();

    std::string msg = std::string("collection: ") + col.c_str() +
      " - query(): " + q.toString() +
      " - exception: generic";

    *err = "Database Error (" + msg + ")";
    alarmMgr.dbError(msg);

    return false;
  }

  alarmMgr.dbErrorReset();
  return true;
}



/* ****************************************************************************
*
* orion::collectionFindOne -
*/
bool orion::collectionFindOne
(
  const std::string&     col,
  const orion::BSONObj&  _q,
  orion::BSONObj*        doc,
  std::string*           err
)
{
  TIME_STAT_MONGO_READ_WAIT_START();

  orion::DBConnection connection = orion::getMongoConnection();

  // Getting the "low level" driver objects
  const mongo::BSONObj q = _q.get();

  if (connection.isNull())
  {
    TIME_STAT_MONGO_READ_WAIT_STOP();

    LM_E(("Fatal Error (null DB connection)"));
    *err = "null DB connection";

    return false;
  }

  LM_T(LmtMongo, ("findOne() in '%s' collection: '%s'", col.c_str(), q.toString().c_str()));
  try
  {
    *doc = orion::BSONObj(connection.get()->findOne(col.c_str(), q));
    orion::releaseMongoConnection(connection);
    TIME_STAT_MONGO_READ_WAIT_STOP();
    LM_I(("Database Operation Successful (findOne: %s)", q.toString().c_str()));
  }
  catch (const std::exception &e)
  {
    orion::releaseMongoConnection(connection);
    TIME_STAT_MONGO_READ_WAIT_STOP();

    std::string msg = std::string("collection: ") + col.c_str() +
        " - findOne(): " + q.toString() +
        " - exception: " + e.what();

    *err = "Database Error (" + msg + ")";
    alarmMgr.dbError(msg);

    return false;
  }
  catch (...)
  {
    orion::releaseMongoConnection(connection);
    TIME_STAT_MONGO_READ_WAIT_STOP();

    std::string msg = std::string("collection: ") + col.c_str() +
        " - findOne(): " + q.toString() +
        " - exception: generic";

    *err = "Database Error (" + msg + ")";
    alarmMgr.dbError(msg);

    return false;
  }

  alarmMgr.dbErrorReset();
  return true;
}



/* ****************************************************************************
*
* orion::collectionInsert -
*/
bool orion::collectionInsert
(
  const std::string&     col,
  const orion::BSONObj&  _doc,
  std::string*           err
)
{
  TIME_STAT_MONGO_WRITE_WAIT_START();

  orion::DBConnection connection = orion::getMongoConnection();

  // Getting the "low level" driver objects
  const mongo::BSONObj doc = _doc.get();

  if (connection.isNull())
  {
    TIME_STAT_MONGO_WRITE_WAIT_STOP();

    LM_E(("Fatal Error (null DB connection)"));
    *err = "null DB connection";

    return false;
  }

  LM_T(LmtMongo, ("insert() in '%s' collection: '%s'", col.c_str(), doc.toString().c_str()));

  try
  {
    connection.get()->insert(col.c_str(), doc);
    orion::releaseMongoConnection(connection);
    TIME_STAT_MONGO_WRITE_WAIT_STOP();
    LM_I(("Database Operation Successful (insert: %s)", doc.toString().c_str()));
  }
  catch (const std::exception &e)
  {
    orion::releaseMongoConnection(connection);
    TIME_STAT_MONGO_WRITE_WAIT_STOP();

    std::string msg = std::string("collection: ") + col.c_str() +
      " - insert(): " + doc.toString() +
      " - exception: " + e.what();

    *err = "Database Error (" + msg + ")";
    alarmMgr.dbError(msg);

    return false;
  }
  catch (...)
  {
    orion::releaseMongoConnection(connection);
    TIME_STAT_MONGO_WRITE_WAIT_STOP();

    std::string msg = std::string("collection: ") + col.c_str() +
      " - insert(): " + doc.toString() +
      " - exception: generic";

    *err = "Database Error (" + msg + ")";
    alarmMgr.dbError(msg);

    return false;
  }

  alarmMgr.dbErrorReset();
  return true;
}



/* ****************************************************************************
*
* orion::collectionUpdate -
*/
bool orion::collectionUpdate
(
  const std::string&     col,
  const orion::BSONObj&  _q,
  const orion::BSONObj&  _doc,
  bool                   upsert,
  std::string*           err
)
{
  TIME_STAT_MONGO_WRITE_WAIT_START();

  orion::DBConnection connection = orion::getMongoConnection();

  // Getting the "low level" driver objects
  const mongo::BSONObj q   = _q.get();
  const mongo::BSONObj doc = _doc.get();

  if (connection.isNull())
  {
    TIME_STAT_MONGO_WRITE_WAIT_STOP();

    LM_E(("Fatal Error (null DB connection)"));
    *err = "null DB connection";

    return false;
  }

  LM_T(LmtMongo, ("update() in '%s' collection: query='%s' doc='%s', upsert=%s",
                  col.c_str(),
                  q.toString().c_str(),
                  doc.toString().c_str(),
                  FT(upsert)));

  try
  {
    connection.get()->update(col.c_str(), q, doc, upsert);
    orion::releaseMongoConnection(connection);
    TIME_STAT_MONGO_WRITE_WAIT_STOP();
    LM_I(("Database Operation Successful (update: <%s, %s>)", q.toString().c_str(), doc.toString().c_str()));
  }
  catch (const std::exception& e)
  {
    orion::releaseMongoConnection(connection);
    TIME_STAT_MONGO_WRITE_WAIT_STOP();

    std::string msg = std::string("collection: ") + col.c_str() +
      " - update(): <" + q.toString() + "," + doc.toString() + ">" +
      " - exception: " + e.what();

    *err = "Database Error (" + msg + ")";
    alarmMgr.dbError(msg);

    return false;
  }
  catch (...)
  {
    orion::releaseMongoConnection(connection);
    TIME_STAT_MONGO_WRITE_WAIT_STOP();

    std::string msg = std::string("collection: ") + col.c_str() +
      " - update(): <" + q.toString() + "," + doc.toString() + ">" +
      " - exception: generic";

    *err = "Database Error (" + msg + ")";
    alarmMgr.dbError(msg);

    return false;
  }
  alarmMgr.dbErrorReset();

  return true;
}



/* ****************************************************************************
*
* orion::collectionRemove -
*/
bool orion::collectionRemove
(
  const std::string&     col,
  const orion::BSONObj&  _q,
  std::string*           err
)
{
  TIME_STAT_MONGO_WRITE_WAIT_START();

  orion::DBConnection connection = orion::getMongoConnection();

  // Getting the "low level" driver objects
  const mongo::BSONObj q = _q.get();

  if (connection.isNull())
  {
    TIME_STAT_MONGO_WRITE_WAIT_STOP();

    LM_E(("Fatal Error (null DB connection)"));
    *err = "null DB connection";

    return false;
  }

  LM_T(LmtMongo, ("remove() in '%s' collection: {%s}", col.c_str(), q.toString().c_str()));

  try
  {
    connection.get()->remove(col.c_str(), q);
    orion::releaseMongoConnection(connection);
    TIME_STAT_MONGO_WRITE_WAIT_STOP();
    LM_I(("Database Operation Successful (remove: %s)", q.toString().c_str()));
  }
  catch (const std::exception &e)
  {
    orion::releaseMongoConnection(connection);
    TIME_STAT_MONGO_WRITE_WAIT_STOP();

    std::string msg = std::string("collection: ") + col.c_str() +
      " - remove(): " + q.toString() +
      " - exception: " + e.what();

    *err = "Database Error (" + msg + ")";
    alarmMgr.dbError(msg);

    return false;
  }
  catch (...)
  {
    orion::releaseMongoConnection(connection);
    TIME_STAT_MONGO_WRITE_WAIT_STOP();

    std::string msg = std::string("collection: ") + col.c_str() +
      " - remove(): " + q.toString() +
      " - exception: generic";

    *err = "Database Error (" + msg + ")";
    alarmMgr.dbError(msg);

    return false;
  }

  alarmMgr.dbErrorReset();
  return true;
}


#if 0
/* ****************************************************************************
*
* collectionCreateIndex -
*/
bool collectionCreateIndex
(
  const std::string&  col,
  const BSONObj&      indexes,
  const bool&         isTTL,
  std::string*        err
)
{
  TIME_STAT_MONGO_COMMAND_WAIT_START();
  DBClientBase* connection = getMongoConnection();

  if (connection == NULL)
  {
    TIME_STAT_MONGO_COMMAND_WAIT_STOP();
    LM_E(("Fatal Error (null DB connection)"));

    return false;
  }

  LM_T(LmtMongo, ("createIndex() in '%s' collection: '%s'", col.c_str(), indexes.toString().c_str()));

  try
  {
    /**
     * Differently from other indexes, a TTL index must contain the "expireAfterSeconds" field set to 0
     * in the query issued to Mongo DB, in order to be defined with an "expireAt" behaviour.
     * This filed is implemented in the Mongo driver with the Index Spec class.
     */
    if (isTTL)
    {
      connection->createIndex(col.c_str(), IndexSpec().addKeys(indexes).expireAfterSeconds(0));
    }
    else
    {
      connection->createIndex(col.c_str(), indexes);
    }

    releaseMongoConnection(connection);
    TIME_STAT_MONGO_COMMAND_WAIT_STOP();
    LM_I(("Database Operation Successful (createIndex: %s)", indexes.toString().c_str()));
  }
  catch (const std::exception &e)
  {
    releaseMongoConnection(connection);
    TIME_STAT_MONGO_COMMAND_WAIT_STOP();

    std::string msg = std::string("collection: ") + col.c_str() +
      " - createIndex(): " + indexes.toString() +
      " - exception: " + e.what();

    *err = "Database Error (" + msg + ")";
    alarmMgr.dbError(msg);

    return false;
  }
  catch (...)
  {
    releaseMongoConnection(connection);
    TIME_STAT_MONGO_COMMAND_WAIT_STOP();

    std::string msg = std::string("collection: ") + col.c_str() +
      " - createIndex(): " + indexes.toString() +
      " - exception: generic";

    *err = "Database Error (" + msg + ")";
    alarmMgr.dbError(msg);

    return false;
  }
  alarmMgr.dbErrorReset();

  return true;
}



/* ****************************************************************************
*
* runCollectionCommand -
*/
bool runCollectionCommand
(
  const std::string&  col,
  const BSONObj&      command,
  BSONObj*            result,
  std::string*        err
)
{
  return runCollectionCommand(NULL, col, command, result, err);
}



/* ****************************************************************************
*
* runCollectionCommand -
*
* NOTE
*   Different from other functions in this module, this function can get the connection
*   in the params, instead of using getMongoConnection().
*   This is only done from DB connection bootstrapping code .
*/
bool runCollectionCommand
(
  DBClientBase*       connection,
  const std::string&  col,
  const BSONObj&      command,
  BSONObj*            result,
  std::string*        err
)
{
  bool releaseConnection = false;

  //
  // The call to TIME_STAT_MONGO_COMMAND_WAIT_START must be on toplevel so that local variables
  // are visible for TIME_STAT_MONGO_COMMAND_WAIT_STOP()
  //
  TIME_STAT_MONGO_COMMAND_WAIT_START();

  if (connection == NULL)
  {
    connection        = getMongoConnection();
    releaseConnection = true;

    if (connection == NULL)
    {
      TIME_STAT_MONGO_COMMAND_WAIT_STOP();
      LM_E(("Fatal Error (null DB connection)"));

      return false;
    }
  }

  LM_T(LmtMongo, ("runCommand() in '%s' collection: '%s'", col.c_str(), command.toString().c_str()));

  try
  {
    connection->runCommand(col.c_str(), command, *result);
    if (releaseConnection)
    {
      releaseMongoConnection(connection);
      TIME_STAT_MONGO_COMMAND_WAIT_STOP();
    }
    LM_I(("Database Operation Successful (command: %s)", command.toString().c_str()));
  }
  catch (const std::exception &e)
  {
    if (releaseConnection)
    {
      releaseMongoConnection(connection);
      TIME_STAT_MONGO_COMMAND_WAIT_STOP();
    }

    std::string msg = std::string("collection: ") + col.c_str() +
      " - runCommand(): " + command.toString() +
      " - exception: " + e.what();

    *err = "Database Error (" + msg + ")";
    alarmMgr.dbError(msg);

    return false;
  }
  catch (...)
  {
    if (releaseConnection)
    {
      releaseMongoConnection(connection);
      TIME_STAT_MONGO_COMMAND_WAIT_STOP();
    }

    std::string msg = std::string("collection: ") + col.c_str() +
      " - runCommand(): " + command.toString() +
      " - exception: generic";

    *err = "Database Error (" + msg + ")";
    alarmMgr.dbError(msg);

    return false;
  }

  alarmMgr.dbErrorReset();
  return true;
}



/* ****************************************************************************
*
* setWriteConcern -
*/
bool setWriteConcern
(
  DBClientBase*        connection,
  const WriteConcern&  wc,
  std::string*         err
)
{
  LM_T(LmtMongo, ("setWritteConcern(): '%d'", wc.nodes()));

  try
  {
    connection->setWriteConcern(wc);
    LM_I(("Database Operation Successful (setWriteConcern: %d)", wc.nodes()));
  }
  catch (const std::exception &e)
  {
    // FIXME: include wc.nodes() in the output message, + operator doesn't work with integers
    std::string msg = std::string("setWritteConcern(): ") + /*wc.nodes() +*/
      " - exception: " + e.what();

    *err = "Database Error (" + msg + ")";
    alarmMgr.dbError(msg);

    return false;
  }
  catch (...)
  {
    // FIXME: include wc.nodes() in the output message, + operator doesn't work with integers
    std::string msg = std::string("setWritteConcern(): ") + /*wc.nodes() + */
      " - exception: generic";

    *err = "Database Error (" + msg + ")";
    alarmMgr.dbError(msg);

    return false;
  }

  alarmMgr.dbErrorReset();
  return true;
}



/* ****************************************************************************
*
* getWriteConcern -
*/
bool getWriteConcern
(
  DBClientBase*  connection,
  WriteConcern*  wc,
  std::string*   err
)
{
  LM_T(LmtMongo, ("getWriteConcern()"));

  try
  {
    *wc = connection->getWriteConcern();
    LM_I(("Database Operation Successful (getWriteConcern)"));
  }
  catch (const std::exception &e)
  {
    std::string msg = std::string("getWritteConern()") +
      " - exception: " + e.what();

    *err = "Database Error (" + msg + ")";
    alarmMgr.dbError(msg);

    return false;
  }
  catch (...)
  {
    std::string msg = std::string("getWritteConern()") +
      " - exception: generic";

    *err = "Database Error (" + msg + ")";
    alarmMgr.dbError(msg);

    return false;
  }

  alarmMgr.dbErrorReset();
  return true;
}



/* ****************************************************************************
*
* connectionAuth -
*/
extern bool connectionAuth
(
  DBClientBase*       connection,
  const std::string&  db,
  const std::string&  user,
  const std::string&  password,
  const std::string&  mechanism,
  std::string*        err
)
{
  try
  {
    mongo::BSONObj params = BSON("mechanism" << mechanism << "user" << user << "db" << db << "pwd" << password);
    connection->auth(params);
  }
  catch (const std::exception &e)
  {
    std::string msg = std::string("authentication fails: db=") + db +
        ", username='" + user + "'" +
        ", password='*****'" +
        ", mechanism='" + mechanism + "'" +
        ", expection='" + e.what() + "'";

    *err = "Database Startup Error (" + msg + ")";
    LM_E((err->c_str()));

    return false;
  }
  catch (...)
  {
    std::string msg = std::string("authentication fails: db=") + db +
        ", username='" + user + "'" +
        ", password='*****'" +
        ", mechanism='" + mechanism + "'" +
        ", expection=generic";

    *err = "Database Startup Error (" + msg + ")";
    LM_E((err->c_str()));

    return false;
  }

  return true;
}
#endif
