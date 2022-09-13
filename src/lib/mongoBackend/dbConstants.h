#ifndef SRC_LIB_MONGOBACKEND_DBCONSTANTS_H_
#define SRC_LIB_MONGOBACKEND_DBCONSTANTS_H_

/*
*
* Copyright 2015 Telefonica Investigacion y Desarrollo, S.A.U
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

/* ***************************************************************************
*
* Constant strings for collections names
*/
#define COL_ENTITIES       "entities"
#define COL_REGISTRATIONS  "registrations"
#define COL_CSUBS          "csubs"



/* ***************************************************************************
*
* Constant strings for field names in collection (first characters
* are the code name: REG_, ENT_ and CSUB_)
*/
#define REG_CONTEXT_REGISTRATION     "contextRegistration"
#define REG_PROVIDING_APPLICATION    "providingApplication"
#define REG_FORWARDING_MODE          "fwdMode"
#define REG_ENTITIES                 "entities"
#define REG_ATTRS                    "attrs"
#define REG_EXPIRATION               "expiration"
#define REG_ENTITY_ID                "id"
#define REG_ENTITY_TYPE              "type"
#define REG_ENTITY_ISPATTERN         "isPattern"
#define REG_ENTITY_ISTYPEPATTERN     "isTypePattern"
#define REG_ATTRS_NAME               "name"
#define REG_ATTRS_TYPE               "type"
#define REG_SERVICE_PATH             "servicePath"
#define REG_FORMAT                   "format"
#define REG_DESCRIPTION              "description"
#define REG_STATUS                   "status"

#define ENT_ATTRS                    "attrs"
#define ENT_ATTRNAMES                "attrNames"
#define ENT_ENTITY_ID                "id"
#define ENT_ENTITY_TYPE              "type"
#define ENT_SERVICE_PATH             "servicePath"
#define ENT_ATTRS_TYPE               "type"
#define ENT_ATTRS_VALUE              "value"
#define ENT_ATTRS_CREATION_DATE      "creDate"
#define ENT_ATTRS_MODIFICATION_DATE  "modDate"
#define ENT_ATTRS_MD                 "md"
#define ENT_ATTRS_MDNAMES            "mdNames"
#define ENT_ATTRS_MD_TYPE            "type"
#define ENT_ATTRS_MD_VALUE           "value"
#define ENT_CREATION_DATE            "creDate"
#define ENT_MODIFICATION_DATE        "modDate"
#define ENT_LOCATION                 "location"
#define ENT_LOCATION_ATTRNAME        "attrName"
#define ENT_LOCATION_COORDS          "coords"
#define ENT_LAST_CORRELATOR          "lastCorrelator"
#define ENT_EXPIRATION               "expDate"

#define CSUB_DESCRIPTION             "description"
#define CSUB_EXPIRATION              "expiration"
#define CSUB_LASTNOTIFICATION        "lastNotification"
#define CSUB_REFERENCE               "reference"
#define CSUB_CONDITIONS              "conditions"
#define CSUB_EXPR                    "expression"
#define CSUB_EXPR_Q                  "q"
#define CSUB_EXPR_MQ                 "mq"
#define CSUB_EXPR_GEOM               "geometry"
#define CSUB_EXPR_COORDS             "coords"
#define CSUB_EXPR_GEOREL             "georel"
#define CSUB_ALTTYPES                "altTypes"

#define CSUB_THROTTLING              "throttling"
#define CSUB_MAXFAILSLIMIT           "maxFailsLimit"
#define CSUB_FAILSCOUNTER            "failsCounter"
#define CSUB_ENTITIES                "entities"
#define CSUB_ATTRS                   "attrs"
#define CSUB_METADATA                "metadata"
#define CSUB_ENTITY_ID               "id"
#define CSUB_ENTITY_TYPE             "type"
#define CSUB_ENTITY_ISPATTERN        "isPattern"
#define CSUB_ENTITY_ISTYPEPATTERN    "isTypePattern"
#define CSUB_COUNT                   "count"
#define CSUB_FORMAT                  "format"
#define CSUB_STATUS                  "status"
#define CSUB_STATUS_LAST_CHANGE      "statusLastChange"
#define CSUB_SERVICE_PATH            "servicePath"
#define CSUB_CUSTOM                  "custom"
#define CSUB_TIMEOUT                 "timeout"
#define CSUB_METHOD                  "method"
#define CSUB_HEADERS                 "headers"
#define CSUB_QS                      "qs"
#define CSUB_PAYLOAD                 "payload"
#define CSUB_JSON                    "json"
#define CSUB_BLACKLIST               "blacklist"
#define CSUB_ONLYCHANGED             "onlyChanged"
#define CSUB_COVERED                 "covered"
#define CSUB_LASTFAILURE             "lastFailure"
#define CSUB_LASTFAILUREASON         "lastFailureReason"
#define CSUB_LASTSUCCESS             "lastSuccess"
#define CSUB_LASTSUCCESSCODE         "lastSuccessCode"

#define CSUB_MQTTTOPIC               "topic"
#define CSUB_MQTTQOS                 "qos"

#define CSUB_USER                    "user"
#define CSUB_PASSWD                  "passwd"



/* ****************************************************************************
*
* Constant strings for field values acting as keywords
*
* FIXME P10: use an enum for active/inactive/expired instead of strings
*/
#define STATUS_ACTIVE        "active"
#define STATUS_INACTIVE      "inactive"
#define STATUS_ONESHOT       "oneshot"



/* ****************************************************************************
*
* Attribute update operators (except $addToSet and $pullAll, which are special ones)
*/
#define UPDATE_OPERATORS_NUMBER 6
const std::string UPDATE_OPERATORS[UPDATE_OPERATORS_NUMBER] = { "$inc", "$min", "$max", "$mul", "$push", "$pull" };

#endif  // SRC_LIB_MONGOBACKEND_DBCONSTANTS_H_
