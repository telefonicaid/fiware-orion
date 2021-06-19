#ifndef SRC_LIB_METRICSMGR_METRICSMANAGER_H_
#define SRC_LIB_METRICSMGR_METRICSMANAGER_H_

/*
*
* Copyright 2016 Telefonica Investigacion y Desarrollo, S.A.U
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
#include <stdint.h>   // int64_t et al
#include <semaphore.h>

#include <utility>
#include <string>
#include <map>



/* ****************************************************************************
*
* Metrics - 
*
* NOTE
*   _METRIC_TOTAL_SERVICE_TIME is in the metrics map but excluded from
*   the metric response, while METRIC_SERVICE_TIME is NOT in the metrics map, but
*   include in the metric response.
*   METRIC_SERVICE_TIME == _METRIC_TOTAL_SERVICE_TIME / METRIC_TRANS_IN
*
*   All 'help-counters' in the set, like _METRIC_TOTAL_SERVICE_TIME will have the
*   prefix '_', to help remember it's a help measure.
*   A prefix'_' in both macro name and in its string translation
*/
#define METRIC_TRANS_IN                            "incomingTransactions"
#define METRIC_TRANS_IN_REQ_SIZE                   "incomingTransactionRequestSize"
#define METRIC_TRANS_IN_RESP_SIZE                  "incomingTransactionResponseSize"
#define METRIC_TRANS_IN_ERRORS                     "incomingTransactionErrors"
#define METRIC_SERVICE_TIME                        "serviceTime"
#define _METRIC_TOTAL_SERVICE_TIME                 "_totalServiceTime"

#define METRIC_TRANS_OUT                           "outgoingTransactions"
#define METRIC_TRANS_OUT_REQ_SIZE                  "outgoingTransactionRequestSize"
#define METRIC_TRANS_OUT_RESP_SIZE                 "outgoingTransactionResponseSize"
#define METRIC_TRANS_OUT_ERRORS                    "outgoingTransactionErrors"

#if 0
//
// The following counters are still under discussion
//
#define METRIC_TRANSACTIONS                        "transactions"
#define METRIC_NGSIV1_TRANSACTIONS                 "ngsiv1Transactions"
#define METRIC_NGSIV2_TRANSACTIONS                 "ngsiv2Transactions"
#define METRIC_AVERAGE_TRANSACTION_TIME            "averageTransactionTime"
#define METRIC_OK_TRANSACTIONS                     "okTransactions"
#define METRIC_BAD_REQUEST                         "badRequestTransactions"
#define METRIC_NOT_FOUND                           "notFoundTransactions"
#define METRIC_INTERNAL_ERROR                      "internalErrorTransactions"

#define METRIC_CREATED_ENTITIES                    "createdEntities"
#define METRIC_UPDATED_ENTITIES                    "updatedEntities"
#define METRIC_DELETED_ENTITIES                    "deletedEntities"
#define METRIC_CREATED_SUBSCRIPTIONS               "createdSubscriptions"
#define METRIC_UPDATED_SUBSCRIPTIONS               "updatedSubscriptions"
#define METRIC_DELETED_SUBSCRIPTIONS               "deletedSubscriptions"
#define METRIC_NOTIFICATIONS_SENT                  "notificationSentOk"
#define METRIC_NOTIFICATIONS_FAILED                "notificationSentFailed"
#define METRIC_CUSTOM_NOTIFICATIONS_SENT           "customNotificationsSentOk"
#define METRIC_CUSTOM_NOTIFICATIONS_FAILED         "customNotificationsSentFailed"

#define METRIC_DB_QUERIES                          "dbQueries"
#define METRIC_DB_REGEXQUERIES                     "dbRegexQueries"
#define METRIC_DB_AGGREGATIONQUERIES               "dbAggregationQueries"
#define METRIC_DB_INSERTS                          "dbInserts"
#define METRIC_DB_UPDATES                          "dbUpdates"
#define METRIC_DB_DELETES                          "dbDeletes"
#define METRIC_DB_QUERY_AVERAGE_TIME               "dbQueryAverageTime"
#define METRIC_DB_REGEX_QUERY_AVERAGE_TIME         "dbRegexQueryAverageTime"
#define METRIC_DB_AGGREGATION_QUERY_AVERAGE_TIME   "dbAggregationQueryAverageTime"
#define METRIC_DB_INSERT_AVERAGE_TIME              "dbInsertAverageTime"
#define METRIC_DB_UPDATE_AVERAGE_TIME              "dbUpdateAverageTime"
#define METRIC_DB_DELETE_AVERAGE_TIME              "dbDeleteAverageTime"

#endif



/* ****************************************************************************
*
* MetricsManager -
*
* FIXME PR: summary of things to improve
*
* 08. Empty services (no tenant given) to receive some default service name.
* 09. What to do with default SP ("/")?
*     When applying the rule "remove the initial /", the default SP ends up as "".
*     Not sure that we want that ...
* 10. About initial slash of service path:
*     The broker accepts a Service Path with empty components, e.g. "//sp1"
*     We understand this is an error that should be fixed so, only the first '/' is removed
*     for metrics
* 11. Try to come up with better solution for metrics for requests using invalid service-path / tenant?
*
*/
class MetricsManager
{
 private:
  std::map<std::string, std::map<std::string, std::map<std::string, uint64_t>*>*>  metrics;
  bool            on;
  sem_t           sem;
  bool            semWaitStatistics;
  int64_t         semWaitTime;        // measured in microseconds

  void            semTake(void);
  void            semGive(void);
  void            _reset(void);
  std::string     _toJson(void);
  bool            serviceValid(const char* srv);
  bool            subServiceValid(const std::string& subsrv);
  bool            servicePathForMetrics(const std::string& spath, std::string* subServiceP);

 public:
  MetricsManager();

  bool         init(bool _on, bool _semWaitStatistics);
  void         add(const char* srv, const char* subServ, const char* metric, uint64_t value);
  void         reset(void);
  std::string  toJson(bool doReset);
  bool         isOn(void);
  int64_t      semWaitTimeGet(void);
  const char*  semStateGet(void);
  void         release(void);
};

#endif  // SRC_LIB_METRICSMGR_METRICSMANAGER_H_
