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
#include <vector>

#include "logMsg/traceLevels.h"
#include "logMsg/logMsg.h"
#include "common/string.h"
#include "common/statistics.h"
#include "common/clockFunctions.h"
#include "alarmMgr/alarmMgr.h"

#include "mongoDriver/connectionOperations.h"
#include "mongoDriver/BSONObjBuilder.h"
#include "mongoDriver/BSONArrayBuilder.h"
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
  const orion::DBConnection&    _connection,
  const std::string&     db,
  const std::string&     col,
  const orion::BSONObj&  _q,
  orion::DBCursor*       cursor,
  std::string*           err
)
{
  std::string ns = db + "." + col;

  // Getting & checking connection
  mongoc_client_t* connection = _connection.get();
  if (connection == NULL)
  {
    LM_E(("Fatal Error (null DB connection)"));
    *err = "null DB connection";

    return false;
  }

  // Get low level driver object
  bson_t* q = _q.get();
  char* bsonStr = bson_as_relaxed_extended_json(q, NULL);

  LM_T(LmtMongo, ("query() in '%s' collection: '%s'", ns.c_str(), bsonStr));
  mongoc_collection_t *collection = mongoc_client_get_collection(connection, db.c_str(), col.c_str());
  mongoc_cursor_t* c = mongoc_collection_find_with_opts(collection, q, NULL, NULL);
  mongoc_collection_destroy(collection);

  // FIXME OLD-DR: pending on PR https://github.com/mongodb/mongo-c-driver/pull/740 merging
  bson_error_t error;
  if (mongoc_cursor_error(c, &error))
  {
    std::string msg = std::string("collection: ") + ns +
      " - query(): " + bsonStr +
      " - exception: " + error.message;

    *err = "Database Error (" + msg + ")";
    alarmMgr.dbError(msg);

    mongoc_cursor_destroy(c);

    return false;
  }

  LM_T(LmtOldInfo, ("Database Operation Successful (query: %s)", bsonStr));

  cursor->set(c);
  alarmMgr.dbErrorReset();

  bson_free(bsonStr);

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
  const orion::DBConnection&  _connection,
  const std::string&         db,
  const std::string&         col,
  const orion::BSONObj&      _q,
  const orion::BSONObj&      _sort,  // FIXME: this change can be propagated independtly to master
  int                        limit,
  int                        offset,
  orion::DBCursor*           cursor,
  long long*                 count,
  std::string*               err
)
{
  std::string ns = db + "." + col;

  // Getting & checking connection
  mongoc_client_t* connection = _connection.get();
  if (connection == NULL)
  {
    LM_E(("Fatal Error (null DB connection)"));
    *err = "null DB connection";

    return false;
  }

  // Getting low level driver object
  bson_t*     q = _q.get();
  char* bsonStr = bson_as_relaxed_extended_json(q, NULL);

  LM_T(LmtMongo, ("query() in '%s' collection limit=%d, offset=%d: '%s'",
                  ns.c_str(), limit, offset, bsonStr));
  mongoc_collection_t *collection = mongoc_client_get_collection(connection, db.c_str(), col.c_str());

  // First, set countP (if used). In the case of error, we return early and the actual query doesn't take place
  if (count != NULL)
  {
    bson_error_t error;
    long long n = mongoc_collection_count_documents(collection, q, NULL, NULL, NULL, &error);
    if (n >= 0)
    {
      *count = n;
    }
    else
    {
      std::string msg = std::string("collection: ") + ns.c_str() +
        " - count_documents(): " + bsonStr +
        " - exception: " + error.message;

      *err = "Database Error (" + msg + ")";
      alarmMgr.dbError(msg);

      mongoc_collection_destroy(collection);
      bson_free(bsonStr);

      return false;
    }
  }

  // Build the options document
  bson_t* opt = bson_new();
  BSON_APPEND_DOCUMENT(opt, "sort", _sort.get());
  BSON_APPEND_INT32(opt, "limit", limit);
  BSON_APPEND_INT32(opt, "skip", offset);

  mongoc_cursor_t* c = mongoc_collection_find_with_opts(collection, q, opt, NULL);

  bson_destroy(opt);
  mongoc_collection_destroy(collection);

  // FIXME OLD-DR: pending on PR https://github.com/mongodb/mongo-c-driver/pull/740 merging
  bson_error_t error;
  if (mongoc_cursor_error(c, &error))
  {
    std::string msg = std::string("collection: ") + ns +
      " - query(): " + bsonStr +
      " - exception: " + error.message;

    *err = "Database Error (" + msg + ")";
    alarmMgr.dbError(msg);

    mongoc_cursor_destroy(c);

    return false;
  }

  LM_T(LmtOldInfo, ("Database Operation Successful (query: %s)", bsonStr));

  cursor->set(c);
  alarmMgr.dbErrorReset();

  bson_free(bsonStr);

  return true;
}



/* ****************************************************************************
*
* orion::collectionCount -
*/
bool orion::collectionCount
(
  const std::string&     db,
  const std::string&     col,
  const orion::BSONObj&  _q,
  unsigned long long*    c,
  std::string*           err
)
{
  TIME_STAT_MONGO_READ_WAIT_START();

  std::string ns = db + "." + col;

  // Getting & checking connection
  orion::DBConnection connection = orion::getMongoConnection();
  if (connection.isNull())
  {
    TIME_STAT_MONGO_READ_WAIT_STOP();

    LM_E(("Fatal Error (null DB connection)"));
    *err = "null DB connection";

    orion::releaseMongoConnection(connection);
    return false;
  }

  // Get low level driver object
  const bson_t* doc = _q.get();
  char* bsonStr = bson_as_relaxed_extended_json(doc, NULL);

  LM_T(LmtMongo, ("count_documents() in '%s' collection: '%s'", ns.c_str(), bsonStr));

  mongoc_collection_t* collection = mongoc_client_get_collection(connection.get(), db.c_str(), col.c_str());

  bson_error_t error;
  long long n = mongoc_collection_count_documents(collection, doc, NULL, NULL, NULL, &error);

  mongoc_collection_destroy(collection);
  orion::releaseMongoConnection(connection);
  TIME_STAT_MONGO_READ_WAIT_STOP();

  if (n >= 0)
  {
    LM_T(LmtOldInfo, ("Database Operation Successful (count_documents: %s)", bsonStr));
    *c = n;
    alarmMgr.dbErrorReset();
  }
  else
  {
    std::string msg = std::string("collection: ") + ns.c_str() +
      " - count_documents(): " + bsonStr +
      " - exception: " + error.message;

    *err = "Database Error (" + msg + ")";
    alarmMgr.dbError(msg);
  }

  bson_free(bsonStr);

  return (n >= 0);
}



/* ****************************************************************************
*
* orion::collectionFindOne -
*
* Note there is no native findOne function in the Mongo C driver,
* see https://stackoverflow.com/questions/66030072/is-there-a-findone-operation-in-the-mongo-c-driver.
* Thus, this function uses orion::collectionRangedQuery() internally
*
* This function return:
*
* - true and err = "": result was found (returned in doc parameter) and no error
* - false and err = "": no result was found and no error in the operation
* - false and err != "": error was found in the operation
*/
bool orion::collectionFindOne
(
  const std::string&     db,
  const std::string&     col,
  const orion::BSONObj&  _q,
  orion::BSONObj*        doc,
  std::string*           err
)
{
  TIME_STAT_MONGO_READ_WAIT_START();

  orion::DBConnection connection = orion::getMongoConnection();

  orion::DBCursor cursor;

  *err = "";

  if (!collectionRangedQuery(connection, db, col, _q, orion::BSONObj(), 1, 0, &cursor, NULL, err))
  {
    orion::releaseMongoConnection(connection);
    TIME_STAT_MONGO_READ_WAIT_STOP();
    return false;
  }

  // Get exactly one item from cursor (checking that at least one result exist)
  BSONObj b;
  bool success = cursor.next(&b);

  if (success)
  {
    // We only return the BSON in doc argument in success case
    *doc = b;
  }

  orion::releaseMongoConnection(connection);
  TIME_STAT_MONGO_READ_WAIT_STOP();

  return success;
}



/* ****************************************************************************
*
* orion::collectionInsert -
*/
bool orion::collectionInsert
(
  const std::string&     db,
  const std::string&     col,
  const orion::BSONObj&  _doc,
  std::string*           err
)
{
  TIME_STAT_MONGO_WRITE_WAIT_START();

  std::string ns = db + "." + col;

  // Getting & checking connection
  orion::DBConnection connection = orion::getMongoConnection();
  if (connection.isNull())
  {
    TIME_STAT_MONGO_WRITE_WAIT_STOP();

    LM_E(("Fatal Error (null DB connection)"));
    *err = "null DB connection";

    orion::releaseMongoConnection(connection);
    return false;
  }

  // Get low level driver object
  const bson_t* doc = _doc.get();
  char* bsonStr = bson_as_relaxed_extended_json(doc, NULL);

  LM_T(LmtMongo, ("insert_one() in '%s' collection: '%s'", ns.c_str(), bsonStr));

  mongoc_collection_t *collection = mongoc_client_get_collection(connection.get(), db.c_str(), col.c_str());

  bson_error_t error;
  bson_t* opt = BCON_NEW("validate", BCON_BOOL(false));
  bool success = mongoc_collection_insert_one(collection, doc, opt, NULL, &error);

  bson_destroy(opt);
  mongoc_collection_destroy(collection);
  orion::releaseMongoConnection(connection);
  TIME_STAT_MONGO_WRITE_WAIT_STOP();

  if (success)
  {
    LM_T(LmtOldInfo, ("Database Operation Successful (insert: %s)", bsonStr));
    alarmMgr.dbErrorReset();
  }
  else
  {
    std::string msg = std::string("collection: ") + ns.c_str() +
      " - insert_one(): " + bsonStr +
      " - exception: " + error.message;

    *err = "Database Error (" + msg + ")";
    alarmMgr.dbError(msg);
  }

  bson_free(bsonStr);

  return success;
}



/* ****************************************************************************
*
* orion::collectionUpdate -
*/
bool orion::collectionUpdate
(
  const std::string&     db,
  const std::string&     col,
  const orion::BSONObj&  _q,
  const orion::BSONObj&  _doc,
  bool                   upsert,
  std::string*           err
)
{
  TIME_STAT_MONGO_WRITE_WAIT_START();

  std::string ns = db + "." + col;

  // Getting & checking connection
  orion::DBConnection connection = orion::getMongoConnection();
  if (connection.isNull())
  {
    TIME_STAT_MONGO_WRITE_WAIT_STOP();

    LM_E(("Fatal Error (null DB connection)"));
    *err = "null DB connection";

    orion::releaseMongoConnection(connection);
    return false;
  }

  // Get log level driver objects
  const bson_t* q   = _q.get();
  const bson_t* doc = _doc.get();
  char* bsonQStr    = bson_as_relaxed_extended_json(q, NULL);
  char* bsonDocStr  = bson_as_relaxed_extended_json(doc, NULL);

  LM_T(LmtMongo, ("update() in '%s' collection: query='%s' doc='%s', upsert=%s",
                  ns.c_str(),
                  bsonQStr,
                  bsonDocStr,
                  FT(upsert)));

  mongoc_collection_t *collection = mongoc_client_get_collection(connection.get(), db.c_str(), col.c_str());

  bson_error_t error;
  bool success = mongoc_collection_update(collection,
                                          upsert ? MONGOC_UPDATE_UPSERT : MONGOC_UPDATE_NONE,
                                          q, doc, NULL, &error);

  mongoc_collection_destroy(collection);
  orion::releaseMongoConnection(connection);
  TIME_STAT_MONGO_WRITE_WAIT_STOP();

  if (success)
  {
    LM_T(LmtOldInfo, ("Database Operation Successful (update: <%s, %s>)", bsonQStr, bsonDocStr));
    alarmMgr.dbErrorReset();
  }
  else
  {
    std::string msg = std::string("collection: ") + ns.c_str() +
      " - update(): <" + bsonQStr + "," + bsonDocStr + ">" +
      " - exception: " + error.message;

    *err = "Database Error (" + msg + ")";
    alarmMgr.dbError(msg);
  }

  bson_free(bsonQStr);
  bson_free(bsonDocStr);

  return success;
}



/* ****************************************************************************
*
* orion::collectionRemove -
*/
bool orion::collectionRemove
(
  const std::string&     db,
  const std::string&     col,
  const orion::BSONObj&  _q,
  std::string*           err
)
{
  TIME_STAT_MONGO_WRITE_WAIT_START();

  std::string ns = db + "." + col;

  // Getting & checking conection
  orion::DBConnection connection = orion::getMongoConnection();
  if (connection.isNull())
  {
    TIME_STAT_MONGO_WRITE_WAIT_STOP();

    LM_E(("Fatal Error (null DB connection)"));
    *err = "null DB connection";

    orion::releaseMongoConnection(connection);
    return false;
  }

  // Get low level driver object
  const bson_t* q = _q.get();
  char* bsonStr = bson_as_relaxed_extended_json(q, NULL);

  LM_T(LmtMongo, ("remove() in '%s' collection: {%s}", ns.c_str(), bsonStr));

  mongoc_collection_t *collection = mongoc_client_get_collection(connection.get(), db.c_str(), col.c_str());

  bson_error_t error;
  bool success = mongoc_collection_remove(collection, MONGOC_REMOVE_NONE, q, NULL, &error);

  mongoc_collection_destroy(collection);
  orion::releaseMongoConnection(connection);
  TIME_STAT_MONGO_WRITE_WAIT_STOP();

  if (success)
  {
    LM_T(LmtOldInfo, ("Database Operation Successful (remove: %s)", bsonStr));
    alarmMgr.dbErrorReset();
  }
  else
  {
    std::string msg = std::string("collection: ") + ns.c_str() +
      " - remove(): " + bsonStr +
      " - exception: " + error.message;

    *err = "Database Error (" + msg + ")";
    alarmMgr.dbError(msg);
  }

  bson_free(bsonStr);

  return success;
}



/* ****************************************************************************
*
* orion::collectionCreateIndex -
*/
bool orion::collectionCreateIndex
(
  const std::string&     db,
  const std::string&     col,
  const std::string&     name,
  const orion::BSONObj&  _index,
  const bool&            isTTL,
  std::string*           err
)
{
  // Recomended way: using commands (see http://mongoc.org/libmongoc/current/create-indexes.html)

  // Prepare command
  orion::BSONObjBuilder    cmd;
  orion::BSONObjBuilder    index;
  orion::BSONArrayBuilder  indexes;

  index.append("key", _index);
  index.append("name", name);
  if (isTTL)
  {
    index.append("expireAfterSeconds", 0);
  }

  indexes.append(index.obj());

  cmd.append("createIndexes", col);
  cmd.append("indexes", indexes.arr());

  // Run index creation (result is irrelevant)
  orion::BSONObj r;
  return runCollectionCommand(db, col, cmd.obj(), &r, err);
}



/* ****************************************************************************
*
* orion::collectionAggregate -
*
*/
bool orion::collectionAggregate
(
  const orion::DBConnection&  _connection,
  const std::string&          db,
  const std::string&          col,
  const orion::BSONArray&     _pipeline,
  unsigned int                batchSize,
  DBCursor*                   cursor,
  std::string*                err
)
{
  std::string ns = db + "." + col;

  // Getting & checking connection
  mongoc_client_t* connection = _connection.get();
  if (connection == NULL)
  {
    LM_E(("Fatal Error (null DB connection)"));
    *err = "null DB connection";

    return false;
  }

  // Get low level driver object
  bson_t* pipeline = _pipeline.get();
  char* bsonStr = bson_array_as_json(pipeline, NULL);

  bson_t* opt = bson_new();
  BSON_APPEND_INT32(opt, "batchSize", batchSize);

  LM_T(LmtMongo, ("aggregate() in '%s' collection: '%s'", ns.c_str(), bsonStr));
  mongoc_collection_t *collection = mongoc_client_get_collection(connection, db.c_str(), col.c_str());

  mongoc_cursor_t* c = mongoc_collection_aggregate(collection, MONGOC_QUERY_NONE, pipeline, opt, NULL);

  bson_destroy(opt);
  mongoc_collection_destroy(collection);

  // FIXME OLD-DR: pending on PR https://github.com/mongodb/mongo-c-driver/pull/740 merging
  bson_error_t error;
  if (mongoc_cursor_error(c, &error))
  {
    std::string msg = std::string("collection: ") + ns +
      " - aggregate(): " + bsonStr +
      " - exception: " + error.message;

    *err = "Database Error (" + msg + ")";
    alarmMgr.dbError(msg);

    mongoc_cursor_destroy(c);

    return false;
  }

  cursor->set(c);

  alarmMgr.dbErrorReset();

  bson_free(bsonStr);

  return true;
}



/* ****************************************************************************
*
* runDatabaseCommand -
*
*/
bool orion::runDatabaseCommand
(
  const std::string&     db,
  const orion::BSONObj&  command,
  orion::BSONObj*        result,
  std::string*           err
)
{
  TIME_STAT_MONGO_COMMAND_WAIT_START();

  // Getting & checking connection
  orion::DBConnection connection = orion::getMongoConnection();
  if (connection.isNull())
  {
    TIME_STAT_MONGO_COMMAND_WAIT_STOP();

    LM_E(("Fatal Error (null DB connection)"));
    *err = "null DB connection";

    orion::releaseMongoConnection(connection);
    return false;
  }

  // Get low level driver object
  const bson_t* doc = command.get();
  char* bsonStr = bson_as_relaxed_extended_json(doc, NULL);

  LM_T(LmtMongo, ("runCommand() in '%s' database: '%s'", db.c_str(), bsonStr));

  mongoc_database_t* database = mongoc_client_get_database(connection.get(), db.c_str());

  bson_t reply;
  bson_error_t error;

  // FIXME #3788: not sure if mongoc_database_command_with_opts is the best choice
  bool success = mongoc_database_command_with_opts(database, doc, NULL, NULL, &reply, &error);

  mongoc_database_destroy(database);
  orion::releaseMongoConnection(connection);
  TIME_STAT_MONGO_COMMAND_WAIT_STOP();

  if (success)
  {
    LM_T(LmtOldInfo, ("Database Operation Successful (command: %s)", bsonStr));
    *result = orion::BSONObj(&reply);
    alarmMgr.dbErrorReset();
  }
  else
  {
    std::string msg = std::string("database: ") + db.c_str() +
      " - runCommand(): " + bsonStr +
      " - exception: " + error.message;

    *err = "Database Error (" + msg + ")";
    alarmMgr.dbError(msg);
  }

  bson_destroy(&reply);
  bson_free(bsonStr);

  return success;
}



/* ****************************************************************************
*
* orion::runCollectionCommand -
*
* FIXME OLD-DR: after the creation of runDatabaseCommand() this function is completely
* unneded
*/
bool orion::runCollectionCommand
(
  const std::string&     db,
  const std::string&     col,
  const orion::BSONObj&  command,
  orion::BSONObj*        result,
  std::string*           err
)
{
  orion::DBConnection connection;

  // Note that connection has a NULL mongo::DBConnection inside, so having the same
  // effect that the version of this method for the old driver.

  return orion::runCollectionCommand(connection, db, col, command, result, err);
}


/* ****************************************************************************
*
* runCollectionCommand -
*
* NOTE
*   Different from other functions in this module, this function can get the connection
*   in the params, instead of using getMongoConnection().
*   This is only done from DB connection bootstrapping code .
*
* FIXME OLD-DR: after the creation of runDatabaseCommand() this function is could
* be modified to ommit 'connection' from parameters and make it similar to other
* fucntions such as collectionInsert, collectionUpdate, etc. Maybe it is not used a this
* moment but it's a good function for a module containing collection operations on DB
*
* FIXME OLD-DR: check if a the end we have calls to this function in client code
* (probably we should include it in mongoDriver API but warng about no-usage in
* a comment in that case)
*/
bool orion::runCollectionCommand
(
  const orion::DBConnection&  _connection,
  const std::string&          db,
  const std::string&          col,
  const orion::BSONObj&       command,
  orion::BSONObj*             result,
  std::string*                err
)
{
  // The call to TIME_STAT_MONGO_COMMAND_WAIT_START must be on toplevel so that local variables
  // are visible for TIME_STAT_MONGO_COMMAND_WAIT_STOP()
  //
  // FIXME OLD-DR: but TIME_STAT_MONGO_COMMAND_WAIT_STOP is not always called... it depends on
  // releaseConnection value. Check original implementation (it's the same)
  //
  TIME_STAT_MONGO_COMMAND_WAIT_START();

  std::string ns = db + "." + col;

  bool releaseConnection = false;

  // FIXME OLD-DR: try to unify both runCollectionCommand functions
  // into just one (the one that needs connection as parameters, assuming not NULL as other functions
  // in this file)

  orion::DBConnection connection;
  if (_connection.isNull())
  {
    connection        = orion::getMongoConnection();
    releaseConnection = true;

    if (connection.isNull())
    {
      TIME_STAT_MONGO_COMMAND_WAIT_STOP();
      LM_E(("Fatal Error (null DB connection)"));

      return false;
    }
  }
  else
  {
    connection = _connection;
  }

  // Get low level driver object
  const bson_t* doc = command.get();
  char* bsonStr = bson_as_relaxed_extended_json(doc, NULL);

  LM_T(LmtMongo, ("runCommand() in '%s' collection: '%s'", ns.c_str(), bsonStr));

  mongoc_collection_t* collection = mongoc_client_get_collection(connection.get(), db.c_str(), col.c_str());

  bson_t reply;
  bson_error_t error;

  // FIXME #3788: not sure if mongoc_collection_command_with_opts is the best choice
  bool success = mongoc_collection_command_with_opts(collection, doc, NULL, NULL, &reply, &error);

  mongoc_collection_destroy(collection);

  if (releaseConnection)
  {
    orion::releaseMongoConnection(connection);
    TIME_STAT_MONGO_COMMAND_WAIT_STOP();
  }

  if (success)
  {
    LM_T(LmtOldInfo, ("Database Operation Successful (command: %s)", bsonStr));
    *result = orion::BSONObj(&reply);
    alarmMgr.dbErrorReset();
  }
  else
  {
    std::string msg = std::string("collection: ") + ns.c_str() +
      " - runCommand(): " + bsonStr +
      " - exception: " + error.message;

    *err = "Database Error (" + msg + ")";
    alarmMgr.dbError(msg);
  }

  bson_destroy(&reply);
  bson_free(bsonStr);

  return success;
}



/* ****************************************************************************
*
* setWriteConcern -
*/
void orion::setWriteConcern(const orion::DBConnection& connection, const orion::WriteConcern& _wc)
{
  mongoc_write_concern_t* wc = mongoc_write_concern_new();

  if (_wc == orion::WCAcknowledged)
  {
    LM_T(LmtMongo, ("setWritteConcern(): 1"));
    mongoc_write_concern_set_w(wc, MONGOC_WRITE_CONCERN_W_DEFAULT);
  }
  else  // orion::WCUnAcknowledged
  {
    LM_T(LmtMongo, ("setWritteConcern(): 0"));
    mongoc_write_concern_set_w(wc, MONGOC_WRITE_CONCERN_W_UNACKNOWLEDGED);
  }

  mongoc_client_set_write_concern(connection.get(), wc);

  mongoc_write_concern_destroy(wc);
}



/* ****************************************************************************
*
* getWriteConcern -
*/
orion::WriteConcern orion::getWriteConcern(const orion::DBConnection& connection)
{
  LM_T(LmtMongo, ("getWriteConcern()"));

  const mongoc_write_concern_t* wc = mongoc_client_get_write_concern(connection.get());

  switch (mongoc_write_concern_get_w(wc))
  {
    case MONGOC_WRITE_CONCERN_W_DEFAULT:
      return orion::WCAcknowledged;
    case MONGOC_WRITE_CONCERN_W_UNACKNOWLEDGED:
      return orion::WCUnAcknowledged;
    default:
      LM_E(("Runtime Error (unknown WC %d, returning Acknowledged as failsafe)", wc));
      return orion::WCAcknowledged;
  }
}


#if 0
/* ****************************************************************************
*
* connectionAuth -
*/
extern bool connectionAuth
(
  const orion::DBConnection&   connection,
  const std::string&           db,
  const std::string&           user,
  const std::string&           password,
  const std::string&           mechanism,
  std::string*                 err
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
