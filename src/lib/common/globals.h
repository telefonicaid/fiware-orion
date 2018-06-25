#ifndef SRC_LIB_COMMON_GLOBALS_H_
#define SRC_LIB_COMMON_GLOBALS_H_

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
* Author: Ken Zangelin
*/
#include <stdint.h>
#include <limits.h>
#include <string>

#include "common/Timer.h"
#include "common/sem.h"



/* ****************************************************************************
*
* FIWARE_LOCATION - 
*/
#define FIWARE_LOCATION             "FIWARE::Location"
#define FIWARE_LOCATION_DEPRECATED  "FIWARE_Location"   // Deprecated (but still supported) in Orion 0.16.0
#define FIWARE_LOCATION_V2          "FIWARE::Location::NGSIv2"

#define EARTH_RADIUS_METERS         6371000

#define LOCATION_WGS84              "WGS84"
#define LOCATION_WGS84_LEGACY       "WSG84"    // We fixed the right string at 0.17.0, but the old one needs to be mantained



/* ****************************************************************************
*
* "geo:" types
*/
#define GEO_POINT    "geo:point"
#define GEO_LINE     "geo:line"
#define GEO_BOX      "geo:box"
#define GEO_POLYGON  "geo:polygon"
#define GEO_JSON     "geo:json"



/* ****************************************************************************
*
* Special orderBy keywords
*/
#define ORDER_BY_PROXIMITY "geo:distance"



/* ****************************************************************************
*
* metadata ID separator
*/
#define MD_ID_SEPARATOR "()"



/* ****************************************************************************
*
* Default Types for entities, attributes and metadata
*/
#define DEFAULT_ENTITY_TYPE       "Thing"
#define DEFAULT_ATTR_STRING_TYPE  "Text"
#define DEFAULT_ATTR_NUMBER_TYPE  "Number"
#define DEFAULT_ATTR_BOOL_TYPE    "Boolean"
#define DEFAULT_ATTR_ARRAY_TYPE   "StructuredValue"
#define DEFAULT_ATTR_OBJECT_TYPE  "StructuredValue"
#define DEFAULT_ATTR_NULL_TYPE    "None"
#define DATE_TYPE                 "DateTime"
#define DATE_TYPE_ALT             "ISO8601"
#define NUMBER_TYPE_ALT           "Quantity"



/* ****************************************************************************
*
* virtual attributes
*/
#define DATE_CREATED    "dateCreated"
#define DATE_MODIFIED   "dateModified"
#define ALL_ATTRS       "*"



/* ****************************************************************************
*
* Transient entity attribute -
*/
#define DATE_EXPIRES "dateExpires"



/* ****************************************************************************
*
* Render modes - 
*/
#define RENDER_MODE_NORMALIZED    "normalized"
#define RENDER_MODE_KEY_VALUES    "keyValues"
#define RENDER_MODE_VALUES        "values"
#define RENDER_MODE_UNIQUE_VALUES "unique"



/* ****************************************************************************
*
* Values for the URI param 'options'
*/
#define OPT_COUNT           "count"
#define OPT_APPEND          "append"
#define OPT_NORMALIZED      "normalized"
#define OPT_VALUES          "values"
#define OPT_KEY_VALUES      "keyValues"
#define OPT_UNIQUE_VALUES   "unique"
#define OPT_DATE_CREATED    DATE_CREATED
#define OPT_DATE_MODIFIED   DATE_MODIFIED
#define OPT_NO_ATTR_DETAIL  "noAttrDetail"
#define OPT_UPSERT          "upsert"



/* ****************************************************************************
*
* NGSIv2 "flavours" to tune some behaviours in mongoBackend -
* 
* It has been suggested to use RequestType enum (in Request.h) instead of this
* of Ngsiv2Flavour. By the moment we see them as separate things (and probably
* flavours will be removed as Orion evolves and NGSIv1 gets removed) but let's
* see how it evolves.
*
* For more detail on this, please have a look to this dicussion at GitHub: 
* https://github.com/telefonicaid/fiware-orion/pull/1706#discussion_r50416202
*/
typedef enum Ngsiv2Flavour
{
  NGSIV2_NO_FLAVOUR               = 0,
  NGSIV2_FLAVOUR_ONCREATE         = 1,
  NGSIV2_FLAVOUR_ONAPPEND         = 2,
  NGSIV2_FLAVOUR_ONUPDATE         = 3
} Ngsiv2Flavour;



/* ****************************************************************************
*
*  NGSI API version -
*/
typedef enum ApiVersion
{
  NO_VERSION  = -1,
  ADMIN_API   = 0,
  V1          = 1,
  V2          = 2
} ApiVersion;



/* ****************************************************************************
*
* PERMANENT_EXPIRES_DATETIME - date for permanent subscriptions/registrations
*/
#define PERMANENT_EXPIRES_DATETIME  LLONG_MAX



/* ****************************************************************************
*
* useful macros
*/
#ifndef MIN
#  define MIN(a, b)     ((a) < (b)? (a) : (b))
#endif

#ifndef MAX
#  define MAX(a, b)     ((a) > (b)? (a) : (b))
#endif



/* ****************************************************************************
*
* OrionExitFunction - 
*/
typedef void (*OrionExitFunction)(int exitCode, const std::string& reason);



/* ****************************************************************************
*
* global variables
*/
extern char               fwdHost[];
extern int                fwdPort;
extern bool               ngsi9Only;
extern bool               harakiri;
extern int                startTime;
extern int                statisticsTime;
extern OrionExitFunction  orionExitFunction;
extern unsigned           cprForwardLimit;
extern char               notificationMode[];
extern bool               noCache;
extern bool               simulatedNotification;

extern bool               semWaitStatistics;
extern bool               timingStatistics;
extern bool               countersStatistics;
extern bool               notifQueueStatistics;

extern bool               checkIdv1;
extern bool               disableCusNotif;

extern bool               insecureNotif;
extern bool               ngsiv1Autocast;



/* ****************************************************************************
*
* orionInit - 
*/
extern void orionInit
(
  OrionExitFunction  exitFunction,
  const char*        version,
  SemOpType          reqPolicy,
  bool               _countersStatistics,
  bool               _semWaitStatistics,
  bool               _timingStatistics,
  bool               _notifQueueStatistics,
  bool               _checkIdv1
);



/* ****************************************************************************
*
* isTrue - 
*/
extern bool isTrue(const std::string& s);
extern bool isFalse(const std::string& s);



/*****************************************************************************
*
* getTimer -
*/
extern Timer* getTimer(void);



/*****************************************************************************
*
* setTimer -
*/
extern void setTimer(Timer* t);



/* ****************************************************************************
*
* getCurrentTime - 
*/ 
extern int getCurrentTime(void);



/* ****************************************************************************
*
* toSeconds -
*/
extern int64_t toSeconds(int value, char what, bool dayPart);



/*****************************************************************************
*
* parse8601 -
*
* This is common code for Duration and Throttling (at least)
*
*/
extern int64_t parse8601(const std::string& s);



/*****************************************************************************
*
* parse8601Time -
*
* This is common code for Duration and Throttling (at least)
*
*/
extern int64_t parse8601Time(const std::string& s);



/* ****************************************************************************
*
* transactionIdGet - 
*
* PARAMETERS
*   readonly:   don't change the transactionId, just return it.
*
* Unless readonly, add one to the transactionId and return it.
* If readonly - just return the current transactionId.
*/
extern int transactionIdGet(bool readonly = true);



/* ****************************************************************************
*
* transactionIdSet - set the transaction ID
*
* To ensure a unique identifier of the transaction, the startTime down to milliseconds
* of the broker is used as prefix (to almost guarantee its uniqueness among brokers)
* Furthermore, a running number is appended for the transaction.
* A 32 bit signed number is used, so its max value is 0x7FFFFFFF (2,147,483,647).
* If the running number overflows, a millisecond is added to the startTime.
*
* The whole thing is stored in the thread variable 'transactionId', supported by the
* logging library 'liblm'.
*
*/
extern void transactionIdSet(void);



/* ****************************************************************************
*
* correlatorIdSet - 
*/
extern void correlatorIdSet(const char* corrId);



/* ****************************************************************************
*
* orderCoordsForBox
*
* It return false in the case of a 'degenerate' box
*
*/
extern bool orderCoordsForBox
(
  double* minLat,
  double* maxLat,
  double* minLon,
  double* maxLon,
  double lat1,
  double lat2,
  double lon1,
  double lon2
);

#endif  // SRC_LIB_COMMON_GLOBALS_H_
