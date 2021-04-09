#ifndef TEST_UNITTESTS_COMMONMOCKS_H_
#define TEST_UNITTESTS_COMMONMOCKS_H_

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
#include <string>
#include <vector>

#include "gmock/gmock.h"

#include "common/globals.h"
#include "common/RenderFormat.h"
#include "mongoBackend/MongoGlobal.h"
#include "apiTypesV2/HttpInfo.h"



#if 0
// FIXME #3775: pending on mongo unit test re-enabling

/* ****************************************************************************
*
* Temporary define for ::testing::_, only for this file
* Note that _ is undeffed after usage
*
* Our style guide forbids the use of "using" in header files (see 
* doc/manuals/contribution_guidelines.md) and we don't want to write out 
* the entire string "::testing::_", as the code gets difficult to read. Thanks 
* to this temporary macro, we can use "_" but without using "using ::testing::_"
*/
#define _ ::testing::_



/* ****************************************************************************
*
* DBClientConnectionMock -
*
* Mock class for mongo::DBClientConnection
*/
class DBClientConnectionMock : public mongo::DBClientConnection
{
 public:
    DBClientConnectionMock()
    {
        /* By default, all methods are redirected to the parent ones. We use the
         * technique described at
         * https://github.com/google/googletest/blob/master/googlemock/docs/CookBook.md#delegating-calls-to-a-parent-class */
        ON_CALL(*this, count(_, _, _, _, _))
                .WillByDefault(::testing::Invoke(this, &DBClientConnectionMock::parent_count));
        ON_CALL(*this, findOne(_, _, _, _))
                .WillByDefault(::testing::Invoke(this, &DBClientConnectionMock::parent_findOne));
        ON_CALL(*this, insert(_, _, _, _))
                .WillByDefault(::testing::Invoke(this, &DBClientConnectionMock::parent_insert));
        ON_CALL(*this, remove(_, _, _, _))
                .WillByDefault(::testing::Invoke(this, &DBClientConnectionMock::parent_remove));
        ON_CALL(*this, update(_, _, _, _, _, _))
                .WillByDefault(::testing::Invoke(this, &DBClientConnectionMock::parent_update));
        ON_CALL(*this, _query(_, _, _, _, _, _, _))
                .WillByDefault(::testing::Invoke(this, &DBClientConnectionMock::parent_query));
        ON_CALL(*this, runCommand(_, _, _, _))
                .WillByDefault(::testing::Invoke(this, &DBClientConnectionMock::parent_runCommand));
    }

    MOCK_METHOD5(count, unsigned long long(const std::string &ns, const mongo::BSONObj &query, int options, int limit, int skip));
    MOCK_METHOD4(findOne, mongo::BSONObj(const std::string &ns, const mongo::Query &query, const mongo::BSONObj *fieldsToReturn, int queryOptions));
    MOCK_METHOD4(insert, void(const std::string &ns, mongo::BSONObj obj, int flags, const mongo::WriteConcern* wc));
    MOCK_METHOD6(update, void(const std::string &ns, mongo::Query query, mongo::BSONObj obj, bool upsert, bool multi, const mongo::WriteConcern* wc));
    MOCK_METHOD4(remove, void(const std::string &ns, mongo::Query q, bool justOne, const mongo::WriteConcern* wc));
    MOCK_METHOD4(runCommand, bool(const std::string &dbname, const mongo::BSONObj& cmd, mongo::BSONObj &info, int options));

    /* We can not directly mock a method that returns std::auto_ptr<T>, so we are using the workaround described in
     * http://stackoverflow.com/questions/7616475/can-google-mock-a-method-with-a-smart-pointer-return-type
     */
    virtual std::auto_ptr<mongo::DBClientCursor> query(const std::string&     ns,
                                                       mongo::Query           query,
                                                       int                    nToReturn,
                                                       int                    nToSkip,
                                                       const mongo::BSONObj*  fieldsToReturn,
                                                       int                    queryOptions,
                                                       int                    batchSize)
    {
        return std::auto_ptr<mongo::DBClientCursor>(_query(ns, query, nToReturn, nToSkip, fieldsToReturn, queryOptions, batchSize));
    }
    MOCK_METHOD7(_query, mongo::DBClientCursor*(const std::string&     ns,
                                                mongo::Query           query,
                                                int                    nToReturn,
                                                int                    nToSkip,
                                                const mongo::BSONObj*  fieldsToReturn,
                                                int                    queryOptions,
                                                int                    batchSize));

    /* Wrappers for parent methods (used in ON_CALL() defaults set in the constructor) */
    unsigned long long parent_count(const std::string &ns, const mongo::BSONObj &query, int options, int limit, int skip)
    {
      return mongo::DBClientConnection::count(ns, query, options, limit, skip);
    }

    mongo::BSONObj parent_findOne(const std::string &ns, const mongo::Query &query, const mongo::BSONObj *fieldsToReturn, int queryOptions)
    {
      return mongo::DBClientConnection::findOne(ns, query, fieldsToReturn, queryOptions);
    }

    void parent_insert(const std::string &ns, mongo::BSONObj obj, int flags, const mongo::WriteConcern* wc)
    {
      return mongo::DBClientConnection::insert(ns, obj, flags, wc);
    }

    void parent_update(const std::string &ns, mongo::Query query, mongo::BSONObj obj, bool upsert, bool multi, const mongo::WriteConcern* wc)
    {
      return mongo::DBClientConnection::update(ns, query, obj, upsert, multi, wc);
    }

    void parent_remove(const std::string &ns, mongo::Query query, bool justOne, const mongo::WriteConcern* wc)
    {
      return mongo::DBClientConnection::remove(ns, query, justOne, wc);
    }

    /* Note that given the way in which query() and _query() are defined, parent_query uses
     * a slightly different pattern
     */
    mongo::DBClientCursor* parent_query(const std::string&     ns,
                                        mongo::Query           query,
                                        int                    nToReturn,
                                        int                    nToSkip,
                                        const mongo::BSONObj*  fieldsToReturn,
                                        int                    queryOptions,
                                        int                    batchSize)
    {
      std::auto_ptr<mongo::DBClientCursor> cursor = mongo::DBClientConnection::query(ns,
                                                                                     query,
                                                                                     nToReturn,
                                                                                     nToSkip,
                                                                                     fieldsToReturn,
                                                                                     queryOptions,
                                                                                     batchSize);
      return cursor.get();
    }

    bool parent_runCommand(const std::string &dbname, const mongo::BSONObj& cmd, mongo::BSONObj &info, int options)
    {
      return mongo::DBClientConnection::runCommand(dbname, cmd, info, options);
    }
};
#undef _



/* ****************************************************************************
*
* DBClientCursorMock -
*
* Mock class for mongo::DBClientCursor
*/
class DBClientCursorMock: public mongo::DBClientCursor
{
 public:
    DBClientCursorMock(mongo::DBClientBase* client, const std::string &_ns, long long _cursorId, int _nToReturn, int options) :
        mongo::DBClientCursor(client, _ns, _cursorId, _nToReturn, options, /*batchSize*/ 10)
    {
      /* By default, all methods are redirected to the parent ones. We use the
       * technique described at
       * http://code.google.com/p/googlemock/wiki/CookBook#Delegating_Calls_to_a_Parent_Class */
      ON_CALL(*this, more())
        .WillByDefault(::testing::Invoke(this, &DBClientCursorMock::parent_more));
      ON_CALL(*this, next())
        .WillByDefault(::testing::Invoke(this, &DBClientCursorMock::parent_next));
    }

    MOCK_METHOD0(more, bool());
    MOCK_METHOD0(next, mongo::BSONObj());

    /* Wrappers for parent methods (used in ON_CALL() defaults set in the constructor) */
    bool parent_more()
    {
      return mongo::DBClientCursor::more();
    }

    mongo::BSONObj parent_next()
    {
      return mongo::DBClientCursor::next();
    }
};
#endif



/* ****************************************************************************
*
* NotifierMock -
*
* Mock class for Notifier
*/
class NotifierMock : public Notifier
{
 public:
  NotifierMock()
  {
    /* By default, all methods are redirected to the parent ones. We use the
     * technique described at
     * http://code.google.com/p/googlemock/wiki/CookBook#Delegating_Calls_to_a_Parent_Class.
     * However, in this case we don't set this, as we don't want threads or notifications
     * actually created/sent
     */
  }

  MOCK_METHOD10(sendNotifyContextRequest, void(NotifyContextRequest&            ncr,
                                              const ngsiv2::HttpInfo&          httpInfo,
                                              const std::string&               tenant,
                                              const std::string&               xauthToken,
                                              const std::string&               fiwareCorrelator,
                                              unsigned int                     correlatorCounter,
                                              RenderFormat                     renderFormat,
                                              const std::vector<std::string>&  attrsFilter,
                                              bool                             blacklist,
                                              const std::vector<std::string>&  metadataFilter));

    /* Wrappers for parent methods (used in ON_CALL() defaults set in the constructor) */
    void parent_sendNotifyContextRequest(NotifyContextRequest&            ncr,
                                         const ngsiv2::HttpInfo&          httpInfo,
                                         const std::string&               tenant,
                                         const std::string&               xauthToken,
                                         const std::string&               fiwareCorrelator,
                                         unsigned int                     correlatorCounter,
                                         RenderFormat                     renderFormat,
                                         const std::vector<std::string>&  attrsFilter,
                                         bool                             blacklist,
                                         const std::vector<std::string>&  metadataFilter)
    {
      Notifier::sendNotifyContextRequest(ncr,
                                         httpInfo,
                                         tenant,
                                         xauthToken,
                                         fiwareCorrelator,
                                         correlatorCounter,
                                         renderFormat,
                                         attrsFilter,
                                         blacklist,
                                         metadataFilter);
    }
};



/* ****************************************************************************
*
* TimerMock -
*
* Mock class for Timer
*/
class TimerMock : public Timer
{
 public:
  TimerMock()
  {
    /* By default, all methods are redirected to the parent ones. We use the
     * technique described at
     * http://code.google.com/p/googlemock/wiki/CookBook#Delegating_Calls_to_a_Parent_Class
     */
    ON_CALL(*this, getCurrentTime())
      .WillByDefault(::testing::Invoke(this, &TimerMock::parent_getCurrentTime));
  }

  MOCK_METHOD0(getCurrentTime, double(void));

  /* Wrappers for parent methods (used in ON_CALL() defaults set in the constructor) */
  double parent_getCurrentTime(void)
  {
    return Timer::getCurrentTime();
  }
};



/* We need a matcher to compare NotifyContextRequest in EXPECT_CALL() for sendNotifyContextRequest, due
 * to NotifyContextRequest is not yet a full fledged object and we can not use the '==' method and
 * Eq() matcher. FIXME */
MATCHER_P(MatchNcr, expected, "")
{
  return matchNotifyContextRequest(expected, &arg);
}



/* We need a matcher to compare NotifyContextAvailabilityRequest in EXPECT_CALL() for sendNotifyContextAvailabilityRequest, due
 * to NotifyContextAvailabilityRequest is not yet a full fledged object and we can not use the '==' method and
 * Eq() matcher. FIXME */
MATCHER_P(MatchNcar, expected, "")
{
  return matchNotifyContextAvailabilityRequest(expected, arg);
}



/* ****************************************************************************
*
* MatchHttpInfo -
*
* NOTE
*   HttpInfo is a class, but after fighting with operator== quite some time I (kz) gave up
*   and tried this way instead.
*/
MATCHER_P(MatchHttpInfo, expected, "")
{
  ngsiv2::HttpInfo* expectedP = (ngsiv2::HttpInfo*) expected;
  ngsiv2::HttpInfo* argP      = (ngsiv2::HttpInfo*) &arg;

  if (expectedP->url != argP->url)
  {
    return false;
  }

  if (expectedP->verb != argP->verb)
  {
    return false;
  }

  if (expectedP->qs != argP->qs)
  {
    return false;
  }

  if (expectedP->headers != argP->headers)
  {
    return false;
  }

  if (expectedP->payload != argP->payload)
  {
    return false;
  }

  if (expectedP->custom != argP->custom)
  {
    return false;
  }

  return true;
}

#endif  // TEST_UNITTESTS_COMMONMOCKS_H_
