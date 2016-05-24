#ifndef COMMON_MOCKS_H
#define COMMON_MOCKS_H

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
#include "gmock/gmock.h"
#include "mongo/client/dbclient.h"
#include "common/globals.h"
#include "common/RenderFormat.h"
#include "mongoBackend/MongoGlobal.h"

using ::testing::_;
using ::testing::Invoke;

/* ****************************************************************************
*
* DBClientConnectionMock -
*
* Mock class for DBClientConnection
*/
class DBClientConnectionMock : public DBClientConnection {

public:

    DBClientConnectionMock() {
        /* By default, all methods are redirected to the parent ones. We use the
         * technique described at
         * http://code.google.com/p/googlemock/wiki/CookBook#Delegating_Calls_to_a_Parent_Class */
        ON_CALL(*this, count(_,_,_,_,_))
                .WillByDefault(Invoke(this, &DBClientConnectionMock::parent_count));
        ON_CALL(*this, findOne(_,_,_,_))
                .WillByDefault(Invoke(this, &DBClientConnectionMock::parent_findOne));
        ON_CALL(*this, insert(_,_,_,_))
                .WillByDefault(Invoke(this, &DBClientConnectionMock::parent_insert));
        ON_CALL(*this, remove(_,_,_,_))
                .WillByDefault(Invoke(this, &DBClientConnectionMock::parent_remove));
        ON_CALL(*this, update(_,_,_,_,_,_))
                .WillByDefault(Invoke(this, &DBClientConnectionMock::parent_update));
        ON_CALL(*this, _query(_,_,_,_,_,_,_))
                .WillByDefault(Invoke(this, &DBClientConnectionMock::parent_query));
        ON_CALL(*this, runCommand(_,_,_,_))
                .WillByDefault(Invoke(this, &DBClientConnectionMock::parent_runCommand));
    }

    MOCK_METHOD5(count, unsigned long long(const std::string &ns, const BSONObj &query, int options, int limit, int skip));
    MOCK_METHOD4(findOne, BSONObj(const std::string &ns, const Query &query, const BSONObj *fieldsToReturn, int queryOptions));
    MOCK_METHOD4(insert, void(const std::string &ns, BSONObj obj, int flags, const WriteConcern* wc));
    MOCK_METHOD6(update, void(const std::string &ns, Query query, BSONObj obj, bool upsert, bool multi, const WriteConcern* wc));
    MOCK_METHOD4(remove, void(const std::string &ns, Query q, bool justOne, const WriteConcern* wc));
    MOCK_METHOD4(runCommand, bool(const std::string &dbname, const BSONObj& cmd, BSONObj &info, int options));

    /* We can not directly mock a method that returns std::auto_ptr<T>, so we are using the workaround described in
     * http://stackoverflow.com/questions/7616475/can-google-mock-a-method-with-a-smart-pointer-return-type */
    virtual std::auto_ptr<DBClientCursor> query(const std::string &ns, Query query, int nToReturn, int nToSkip, const BSONObj *fieldsToReturn, int queryOptions, int batchSize)
    {
        return std::auto_ptr<DBClientCursor>(_query(ns, query, nToReturn, nToSkip, fieldsToReturn, queryOptions, batchSize));
    }
    MOCK_METHOD7(_query, DBClientCursor*(const std::string &ns, Query query, int nToReturn, int nToSkip, const BSONObj *fieldsToReturn, int queryOptions, int batchSize));

    /* Wrappers for parent methods (used in ON_CALL() defaults set in the constructor) */
    unsigned long long parent_count(const std::string &ns, const BSONObj &query, int options, int limit, int skip) {
        return DBClientConnection::count(ns, query, options, limit, skip);
    }
    BSONObj parent_findOne(const std::string &ns, const Query &query, const BSONObj *fieldsToReturn, int queryOptions) {
        return DBClientConnection::findOne(ns, query, fieldsToReturn, queryOptions);
    }
    void parent_insert(const std::string &ns, BSONObj obj, int flags, const WriteConcern* wc) {
        return DBClientConnection::insert(ns, obj, flags, wc);
    }
    void parent_update(const std::string &ns, Query query, BSONObj obj, bool upsert, bool multi, const WriteConcern* wc) {
        return DBClientConnection::update(ns, query, obj, upsert, multi, wc);
    }
    void parent_remove(const std::string &ns, Query query, bool justOne, const WriteConcern* wc) {
        return DBClientConnection::remove(ns, query, justOne, wc);
    }
    /* Note that given the way in which query() and _query() are defined, parent_query uses
     * a slightly different pattern */
    DBClientCursor* parent_query(const std::string &ns, Query query, int nToReturn, int nToSkip, const BSONObj *fieldsToReturn, int queryOptions, int batchSize) {
        std::auto_ptr<DBClientCursor> cursor = DBClientConnection::query(ns, query, nToReturn, nToSkip, fieldsToReturn, queryOptions, batchSize);
        return cursor.get();
    }
    bool parent_runCommand(const std::string &dbname, const BSONObj& cmd,BSONObj &info, int options)
    {
        return DBClientConnection::runCommand(dbname, cmd, info, options);
    }

};

/* ****************************************************************************
*
* DBClientCursorMock -
*
* Mock class for DBClientCursor
*/
class DBClientCursorMock: public DBClientCursor {

public:

    DBClientCursorMock(DBClientBase *client, const std::string &_ns, long long _cursorId, int _nToReturn, int options) :
        DBClientCursor(client, _ns, _cursorId, _nToReturn, options, /*batchSize*/ 10)
    {
        /* By default, all methods are redirected to the parent ones. We use the
         * technique described at
         * http://code.google.com/p/googlemock/wiki/CookBook#Delegating_Calls_to_a_Parent_Class */
        ON_CALL(*this, more())
                .WillByDefault(Invoke(this, &DBClientCursorMock::parent_more));
        ON_CALL(*this, next())
                .WillByDefault(Invoke(this, &DBClientCursorMock::parent_next));
    }

    MOCK_METHOD0(more, bool());
    MOCK_METHOD0(next, BSONObj());

    /* Wrappers for parent methods (used in ON_CALL() defaults set in the constructor) */
    bool parent_more() {
        return DBClientCursor::more();
    }
    BSONObj parent_next() {
        return DBClientCursor::next();
    }

};

/* ****************************************************************************
*
* NotifierMock -
*
* Mock class for Notifier
*/
class NotifierMock : public Notifier {

public:

    NotifierMock() {
        /* By default, all methods are redirected to the parent ones. We use the
         * technique described at
         * http://code.google.com/p/googlemock/wiki/CookBook#Delegating_Calls_to_a_Parent_Class.
         * However, in this case we don't set this, as we don't want threads or notifications
         * actually created/sent */
    }

    MOCK_METHOD8(sendNotifyContextRequest, void(NotifyContextRequest* ncr, const std::string& url, const std::string& tenant, const std::string& xauthToken, const std::string& fiwareCorrelator, RenderFormat renderFormat, const std::vector<std::string>&  attrsFilter, bool blacklist));
    MOCK_METHOD5(sendNotifyContextAvailabilityRequest, void(NotifyContextAvailabilityRequest* ncar, const std::string& url, const std::string& tenant, const std::string& fiwareCorrelator, RenderFormat renderFormat));

    /* Wrappers for parent methods (used in ON_CALL() defaults set in the constructor) */
    void parent_sendNotifyContextRequest(NotifyContextRequest* ncr, const std::string& url, const std::string& tenant, const std::string& xauthToken, const std::string& fiwareCorrelator, RenderFormat renderFormat, const std::vector<std::string>&  attrsFilter)
    {
      Notifier::sendNotifyContextRequest(ncr, url, tenant, xauthToken, fiwareCorrelator, renderFormat, attrsFilter);
    }
    void parent_sendNotifyContextAvailabilityRequest(NotifyContextAvailabilityRequest* ncar, const std::string& url, const std::string& tenant, const std::string& fiwareCorrelator, RenderFormat renderFormat)
    {
      Notifier::sendNotifyContextAvailabilityRequest(ncar, url, tenant, fiwareCorrelator, renderFormat);
    }
};

/* ****************************************************************************
*
* TimerMock -
*
* Mock class for Timer
*/
class TimerMock : public Timer {

public:

   TimerMock() {
        /* By default, all methods are redirected to the parent ones. We use the
         * technique described at
         * http://code.google.com/p/googlemock/wiki/CookBook#Delegating_Calls_to_a_Parent_Class */
         ON_CALL(*this, getCurrentTime())
               .WillByDefault(Invoke(this, &TimerMock::parent_getCurrentTime));
    }

    MOCK_METHOD0(getCurrentTime, int(void));

    /* Wrappers for parent methods (used in ON_CALL() defaults set in the constructor) */
    int parent_getCurrentTime(void) {
        return Timer::getCurrentTime();
    }

};

/* We need a matcher to compare NotifyContextRequest in EXPECT_CALL() for sendNotifyContextRequest, due
 * to NotifyContextRequest is not yet a full fledged object and we can not use the '==' method and
 * Eq() matcher. FIXME */
MATCHER_P(MatchNcr, expected, "") {

    return matchNotifyContextRequest(expected, arg);

}

/* We need a matcher to compare NotifyContextAvailabilityRequest in EXPECT_CALL() for sendNotifyContextAvailabilityRequest, due
 * to NotifyContextAvailabilityRequest is not yet a full fledged object and we can not use the '==' method and
 * Eq() matcher. FIXME */
MATCHER_P(MatchNcar, expected, "") {

    return matchNotifyContextAvailabilityRequest(expected, arg);

}

#endif

