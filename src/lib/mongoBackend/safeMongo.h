#ifndef SRC_LIB_MONGOBACKEND_SAFEBSONGET_H_
#define SRC_LIB_MONGOBACKEND_SAFEBSONGET_H_

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
* Author: Fermin Galan Marquez
*/

#include "ngsi/SubscriptionId.h"
#include "ngsi/RegistrationId.h"
#include "ngsi/StatusCode.h"

#include "mongo/client/dbclient.h"

/* ****************************************************************************
*
* getObjectField -
*/
extern mongo::BSONObj getObjectField(const mongo::BSONObj& b, const std::string& field, const std::string& caller = "<none>");

/* ****************************************************************************
*
* getStringField -
*/
extern std::string getStringField(const mongo::BSONObj& b, const std::string& field, const std::string& caller = "<none>");

/* ****************************************************************************
*
* getIntField -
*/
extern int getIntField(const mongo::BSONObj& b, const std::string& field, const std::string& caller = "<none>");

/* ****************************************************************************
*
* getLongField -
*/
extern long long getLongField(const mongo::BSONObj& b, const std::string& field, const std::string& caller = "<none>");

/* ****************************************************************************
*
* getIntOrLongFieldAsLong -
*/
extern long long getIntOrLongFieldAsLong(const mongo::BSONObj& b, const std::string& field, const std::string& caller = "<none>");

/* ****************************************************************************
*
* getBoolField -
*/
extern bool getBoolField(const mongo::BSONObj& b, const std::string& field, const std::string& caller = "<none>");

/* ****************************************************************************
*
* getField -
*/
extern mongo::BSONElement getField(const mongo::BSONObj& b, const std::string& field, const std::string& caller = "<none>");

/* ****************************************************************************
*
* moreSafe -
*/
extern bool moreSafe(const std::auto_ptr<mongo::DBClientCursor>& cursor);

/* ****************************************************************************
*
* nextSafeOrError -
*
*/
extern bool nextSafeOrError(const std::auto_ptr<mongo::DBClientCursor>& cursor, mongo::BSONObj* r, std::string* err);

/* ****************************************************************************
*
* safeGetSubId -
*
*/
extern bool safeGetSubId(const SubscriptionId& subId, mongo::OID* id, StatusCode* sc);

/* ****************************************************************************
*
* safeGetRegId -
*
*/
extern bool safeGetRegId(const RegistrationId& regId, mongo::OID* id, StatusCode* sc);

#endif // SRC_LIB_MONGOBACKEND_SAFEBSONGET_H_
