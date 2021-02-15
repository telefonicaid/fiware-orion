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



#if 0
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
    LM_T(LmtOldInfo, ("Database Operation Successful (query: %s)", q.toString().c_str()));
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
#endif



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
  // FIXME OLD-DR: change function signature to have db and col separately and remove
  // this tokenization logic
  std::stringstream ss(col);
  std::vector<std::string> tokens;
  while(ss.good())
  {
    std::string substr;
    getline(ss, substr, '.');
    tokens.push_back(substr);
  }

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

  LM_T(LmtMongo, ("query() in '%s' collection: '%s'", col.c_str(), bsonStr));
  mongoc_collection_t *collection = mongoc_client_get_collection(connection, tokens[0].c_str(), tokens[1].c_str());
  cursor->set(mongoc_collection_find_with_opts(collection, q, NULL, NULL));
  mongoc_collection_destroy(collection);

  // FIXME OLD-DR: how to manage errors? this function doesn't use bson_error_t...
  // Is enough just checking for return cursor pointer NULL-ness? Note that
  // doc http://mongoc.org/libmongoc/1.17.3/mongoc_collection_find_with_opts.html
  // doesn't describes NULL as a possible return value...
  // Asked: https://stackoverflow.com/questions/66027858/how-to-get-errors-when-calling-mongoc-collection-update-function
  if (cursor->isNull())
  {
    std::string msg = std::string("collection: ") + col +
      " - query(): " + bsonStr +
      " - exception: Null cursor from mongo (details on this is found in the source code";

    *err = "Database Error (" + msg + ")";
    alarmMgr.dbError(msg);

    return false;
  }

  alarmMgr.dbErrorReset();

  bson_free(bsonStr);

  return true;
}



#if 0
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
    LM_T(LmtOldInfo, ("Database Operation Successful (query: %s)", query.toString().c_str()));
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
#endif



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
  // FIXME OLD-DR: change function signature to have db and col separately and remove
  // this tokenization logic
  std::stringstream ss(col);
  std::vector<std::string> tokens;
  while(ss.good())
  {
    std::string substr;
    getline(ss, substr, '.');
    tokens.push_back(substr);
  }

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
                  col.c_str(), limit, offset, bsonStr));
  mongoc_collection_t *collection = mongoc_client_get_collection(connection, tokens[0].c_str(), tokens[1].c_str());

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
      std::string msg = std::string("collection: ") + col.c_str() +
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

  cursor->set(mongoc_collection_find_with_opts(collection, q, opt, NULL));

  bson_destroy(opt);
  mongoc_collection_destroy(collection);

  // FIXME OLD-DR: how to manage errors? this function doesn't use bson_error_t...
  // Is enough just checking for return cursor pointer NULL-ness? Note that
  // doc http://mongoc.org/libmongoc/1.17.3/mongoc_collection_find_with_opts.html
  // doesn't describes NULL as a possible return value...
  // Asked: https://stackoverflow.com/questions/66027858/how-to-get-errors-when-calling-mongoc-collection-update-function
  if (cursor->isNull())
  {
    std::string msg = std::string("collection: ") + col +
      " - query(): " + bsonStr +
      " - exception: Null cursor from mongo (details on this is found in the source code";

    *err = "Database Error (" + msg + ")";
    alarmMgr.dbError(msg);

    return false;
  }

  LM_T(LmtOldInfo, ("Database Operation Successful (query: %s)", bsonStr));

  alarmMgr.dbErrorReset();

  bson_free(bsonStr);

  return true;
}



#if 0
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
    LM_T(LmtOldInfo, ("Database Operation Successful (count: %s)", q.toString().c_str()));
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
#endif



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

  // FIXME OLD-DR: change function signature to have db and col separately and remove
  // this tokenization logic
  std::stringstream ss(col);
  std::vector<std::string> tokens;
  while(ss.good())
  {
    std::string substr;
    getline(ss, substr, '.');
    tokens.push_back(substr);
  }

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

  LM_T(LmtMongo, ("count_documents() in '%s' collection: '%s'", col.c_str(), bsonStr));

  mongoc_collection_t* collection = mongoc_client_get_collection(connection.get(), tokens[0].c_str(), tokens[1].c_str());

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
    std::string msg = std::string("collection: ") + col.c_str() +
      " - count_documents(): " + bsonStr +
      " - exception: " + error.message;

    *err = "Database Error (" + msg + ")";
    alarmMgr.dbError(msg);
  }

  bson_free(bsonStr);

  return (n >= 0);
}


#if 0
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
    LM_T(LmtOldInfo, ("Database Operation Successful (findOne: %s)", q.toString().c_str()));
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
#endif



/* ****************************************************************************
*
* orion::collectionFindOne -
*
* FIXME OLD-DR: is seems there is no findOne native operation in the driver
* (asked at https://stackoverflow.com/questions/66030072/is-there-a-findone-operation-in-the-mongo-c-driver)
* By the moment this function implements findOne on top of collectionFindRangedQuery
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

  orion::DBCursor cursor;

  if (!collectionRangedQuery(connection, col, _q, orion::BSONObj(), 1, 0, &cursor, NULL, err))
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



#if 0
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
    LM_T(LmtOldInfo, ("Database Operation Successful (insert: %s)", doc.toString().c_str()));
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
#endif



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

  // FIXME OLD-DR: change function signature to have db and col separately and remove
  // this tokenization logic
  std::stringstream ss(col);
  std::vector<std::string> tokens;
  while(ss.good())
  {
    std::string substr;
    getline(ss, substr, '.');
    tokens.push_back(substr);
  }

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

  LM_T(LmtMongo, ("insert_one() in '%s' collection: '%s'", col.c_str(), bsonStr));

  mongoc_collection_t *collection = mongoc_client_get_collection(connection.get(), tokens[0].c_str(), tokens[1].c_str());

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
    std::string msg = std::string("collection: ") + col.c_str() +
      " - insert_one(): " + bsonStr +
      " - exception: " + error.message;

    *err = "Database Error (" + msg + ")";
    alarmMgr.dbError(msg);
  }

  bson_free(bsonStr);

  return success;
}


#if 0
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
    LM_T(LmtOldInfo, ("Database Operation Successful (update: <%s, %s>)", q.toString().c_str(), doc.toString().c_str()));
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
#endif



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

  // FIXME OLD-DR: change function signature to have db and col separately and remove
  // this tokenization logic
  std::stringstream ss(col);
  std::vector<std::string> tokens;
  while(ss.good())
  {
    std::string substr;
    getline(ss, substr, '.');
    tokens.push_back(substr);
  }

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
  char* bsonQStr   = bson_as_relaxed_extended_json(q, NULL);
  char* bsonDocStr = bson_as_relaxed_extended_json(doc, NULL);

  LM_T(LmtMongo, ("update() in '%s' collection: query='%s' doc='%s', upsert=%s",
                  col.c_str(),
                  bsonQStr,
                  bsonDocStr,
                  FT(upsert)));

  mongoc_collection_t *collection = mongoc_client_get_collection(connection.get(), tokens[0].c_str(), tokens[1].c_str());

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
    std::string msg = std::string("collection: ") + col.c_str() +
      " - update(): <" + bsonQStr + "," + bsonDocStr + ">" +
      " - exception: " + error.message;

    *err = "Database Error (" + msg + ")";
    alarmMgr.dbError(msg);
  }

  bson_free(bsonQStr);
  bson_free(bsonDocStr);

  return success;
}


#if 0
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
    LM_T(LmtOldInfo, ("Database Operation Successful (remove: %s)", q.toString().c_str()));
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
#endif



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

  // FIXME OLD-DR: change function signature to have db and col separately and remove
  // this tokenization logic
  std::stringstream ss(col);
  std::vector<std::string> tokens;
  while(ss.good())
  {
    std::string substr;
    getline(ss, substr, '.');
    tokens.push_back(substr);
  }

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

  LM_T(LmtMongo, ("remove() in '%s' collection: {%s}", col.c_str(), bsonStr));

  mongoc_collection_t *collection = mongoc_client_get_collection(connection.get(), tokens[0].c_str(), tokens[1].c_str());

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
    std::string msg = std::string("collection: ") + col.c_str() +
      " - remove(): " + bsonStr +
      " - exception: " + error.message;

    *err = "Database Error (" + msg + ")";
    alarmMgr.dbError(msg);
  }

  bson_free(bsonStr);

  return success;
}



#if 0
/* ****************************************************************************
*
* orion::collectionCreateIndex -
*/
bool orion::collectionCreateIndex
(
  const std::string&     col,
  const orion::BSONObj&  _indexes,
  const bool&            isTTL,
  std::string*           err
)
{
  TIME_STAT_MONGO_COMMAND_WAIT_START();

  orion::DBConnection connection = orion::getMongoConnection();

  // Getting the "low level" driver objects
  const mongo::BSONObj indexes = _indexes.get();

  if (connection.isNull())
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
      connection.get()->createIndex(col.c_str(), mongo::IndexSpec().addKeys(indexes).expireAfterSeconds(0));
    }
    else
    {
      connection.get()->createIndex(col.c_str(), indexes);
    }

    orion::releaseMongoConnection(connection);
    TIME_STAT_MONGO_COMMAND_WAIT_STOP();
    LM_T(LmtOldInfo, ("Database Operation Successful (createIndex: %s)", indexes.toString().c_str()));
  }
  catch (const std::exception &e)
  {
    orion::releaseMongoConnection(connection);
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
    orion::releaseMongoConnection(connection);
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
#endif



/* ****************************************************************************
*
* orion::collectionCreateIndex -
*/
bool orion::collectionCreateIndex
(
  const std::string&     col,
  const std::string&     name,
  const orion::BSONObj&  _index,
  const bool&            isTTL,
  std::string*           err
)
{
  // Recomended way: using commands (see http://mongoc.org/libmongoc/current/create-indexes.html)

  // FIXME OLD-DR: change function signature to have db and col separately and remove
  // this tokenization logic
  std::stringstream ss(col);
  std::vector<std::string> tokens;
  while(ss.good())
  {
    std::string substr;
    getline(ss, substr, '.');
    tokens.push_back(substr);
  }

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

  cmd.append("createIndexes", tokens[1]);
  cmd.append("indexes", indexes.arr());

  // Run index creation (result is irrelevant)
  orion::BSONObj r;
  return runCollectionCommand(col, cmd.obj(), &r, err);
}



#if 0
/* ****************************************************************************
*
* orion::runCollectionCommand -
*/
bool orion::runCollectionCommand
(
  const std::string&     col,
  const orion::BSONObj&  command,
  orion::BSONObj*        result,
  std::string*           err
)
{
  orion::DBConnection connection;

  // Note that connection has a NULL mongo::DBConnection inside, so having the same
  // effect that the version of this method for the old driver.

  return orion::runCollectionCommand(connection, col, command, result, err);
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
bool orion::runCollectionCommand
(
  orion::DBConnection    connection,
  const std::string&     col,
  const orion::BSONObj&  command,
  orion::BSONObj*        result,
  std::string*           err
)
{
  bool releaseConnection = false;

  //
  // The call to TIME_STAT_MONGO_COMMAND_WAIT_START must be on toplevel so that local variables
  // are visible for TIME_STAT_MONGO_COMMAND_WAIT_STOP()
  //
  TIME_STAT_MONGO_COMMAND_WAIT_START();

  // FIXME OLD-DR: re-assign a veriable that comes from the function parameters is weird...
  // probably this should be improved. Maybe a solution is to unify both runCollectionCommand functions
  // into just one (the one that needs connection as parameters, assuming not NULL as other functions
  // in this file)

  if (connection.isNull())
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

  LM_T(LmtMongo, ("runCommand() in '%s' collection: '%s'", col.c_str(), command.toString().c_str()));

  mongo::BSONObj mResult;
  try
  {
    connection.get()->runCommand(col.c_str(), command.get(), mResult);
    if (releaseConnection)
    {
      orion::releaseMongoConnection(connection);
      TIME_STAT_MONGO_COMMAND_WAIT_STOP();
    }
    LM_T(LmtOldInfo, ("Database Operation Successful (command: %s)", command.toString().c_str()));
  }
  catch (const std::exception &e)
  {
    if (releaseConnection)
    {
      orion::releaseMongoConnection(connection);
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
      orion::releaseMongoConnection(connection);
      TIME_STAT_MONGO_COMMAND_WAIT_STOP();
    }

    std::string msg = std::string("collection: ") + col.c_str() +
      " - runCommand(): " + command.toString() +
      " - exception: generic";

    *err = "Database Error (" + msg + ")";
    alarmMgr.dbError(msg);

    return false;
  }

  *result = orion::BSONObj(mResult);
  alarmMgr.dbErrorReset();
  return true;
}
#endif


/* ****************************************************************************
*
* orion::collectionAggregate -
*
*/
bool orion::collectionAggregate
(
  orion::DBConnection      _connection,
  const std::string&       col,
  const orion::BSONArray&  _pipeline,
  unsigned int             batchSize,
  DBCursor*                cursor,
  std::string*             err
)
{
  // FIXME OLD-DR: change function signature to have db and col separately and remove
  // this tokenization logic
  std::stringstream ss(col);
  std::vector<std::string> tokens;
  while(ss.good())
  {
    std::string substr;
    getline(ss, substr, '.');
    tokens.push_back(substr);
  }

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

  LM_T(LmtMongo, ("aggregate() in '%s' collection: '%s'", col.c_str(), bsonStr));
  mongoc_collection_t *collection = mongoc_client_get_collection(connection, tokens[0].c_str(), tokens[1].c_str());

  cursor->set(mongoc_collection_aggregate(collection, MONGOC_QUERY_NONE, pipeline, opt, NULL));

  bson_destroy(opt);
  mongoc_collection_destroy(collection);

  // FIXME OLD-DR: how to manage errors? this function doesn't use bson_error_t...
  // Is enough just checking for return cursor pointer NULL-ness? Note that
  // doc http://mongoc.org/libmongoc/1.17.3/mongoc_collection_find_with_opts.html
  // doesn't describes NULL as a possible return value...
  // Asked: https://stackoverflow.com/questions/66027858/how-to-get-errors-when-calling-mongoc-collection-update-function
  if (cursor->isNull())
  {
    std::string msg = std::string("collection: ") + col +
      " - aggregate(): " + bsonStr +
      " - exception: Null cursor from mongo (details on this is found in the source code";

    *err = "Database Error (" + msg + ")";
    alarmMgr.dbError(msg);

    return false;
  }

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

  // FIXME OLD-DR: there is also a function mongoc_database_write_command_with_opts, pretty similar
  // but for write operations  (this is the one we use for createIndex). As far as I checked, all
  // the usages are for read operations (aggregate, listDatabases and buildInfo), but maybe we should
  // make the name of the function more explicit, i.e. runCollectionReadCommand
  // Moreover, there are more functions for command... Unclear. Asked (for collections commands, but
  // I guess it's pretty much the same) here: https://stackoverflow.com/questions/66031779/several-functions-in-mongo-c-driver-to-run-commands-on-collections-in-which-cas
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
  const std::string&     col,
  const orion::BSONObj&  command,
  orion::BSONObj*        result,
  std::string*           err
)
{
  orion::DBConnection connection;

  // Note that connection has a NULL mongo::DBConnection inside, so having the same
  // effect that the version of this method for the old driver.

  return orion::runCollectionCommand(connection, col, command, result, err);
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
* FIXME OLD-DR: check if a the end we have callas to this function in client code
* (probably we should include it in mongoDriver API but warng about no-usage in
* a comment in that case)
*/
bool orion::runCollectionCommand
(
  orion::DBConnection    connection,
  const std::string&     col,
  const orion::BSONObj&  command,
  orion::BSONObj*        result,
  std::string*           err
)
{
  bool releaseConnection = false;

  //
  // The call to TIME_STAT_MONGO_COMMAND_WAIT_START must be on toplevel so that local variables
  // are visible for TIME_STAT_MONGO_COMMAND_WAIT_STOP()
  //
  // FIXME OLD-DR: but TIME_STAT_MONGO_COMMAND_WAIT_STOP is not always called... it depends on
  // releaseConnection value. Check original implementation (it's the same)
  //
  TIME_STAT_MONGO_COMMAND_WAIT_START();

  // FIXME OLD-DR: change function signature to have db and col separately and remove
  // this tokenization logic
  std::stringstream ss(col);
  std::vector<std::string> tokens;
  while(ss.good())
  {
    std::string substr;
    getline(ss, substr, '.');
    tokens.push_back(substr);
  }

  // FIXME OLD-DR: re-assign a veriable that comes from the function parameters is weird...
  // probably this should be improved. Maybe a solution is to unify both runCollectionCommand functions
  // into just one (the one that needs connection as parameters, assuming not NULL as other functions
  // in this file)

  if (connection.isNull())
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

  // Get low level driver object
  const bson_t* doc = command.get();
  char* bsonStr = bson_as_relaxed_extended_json(doc, NULL);

  LM_T(LmtMongo, ("runCommand() in '%s' collection: '%s'", col.c_str(), bsonStr));

  mongoc_collection_t* collection = mongoc_client_get_collection(connection.get(), tokens[0].c_str(), tokens[1].c_str());

  bson_t reply;
  bson_error_t error;

  // FIXME OLD-DR: there is also a function mongoc_collection_write_command_with_opts, pretty similar
  // but for write operations  (this is the one we use for createIndex). As far as I checked, all
  // the usages are for read operations (aggregate, listDatabases and buildInfo), but maybe we should
  // make the name of the function more explicit, i.e. runCollectionReadCommand
  // Moreover, there are more functions for command... Unclear. Asked here: https://stackoverflow.com/questions/66031779/several-functions-in-mongo-c-driver-to-run-commands-on-collections-in-which-cas
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
    std::string msg = std::string("collection: ") + col.c_str() +
      " - runCommand(): " + bsonStr +
      " - exception: " + error.message;

    *err = "Database Error (" + msg + ")";
    alarmMgr.dbError(msg);
  }

  bson_destroy(&reply);
  bson_free(bsonStr);

  return success;
}


#if 0
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
    LM_T(LmtOldInfo, ("Database Operation Successful (setWriteConcern: %d)", wc.nodes()));
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
    LM_T(LmtOldInfo, ("Database Operation Successful (getWriteConcern)"));
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
